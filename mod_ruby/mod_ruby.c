/*
 * $Id$
 * Copyright (C) 2000  ZetaBITS, Inc.
 * Copyright (C) 2000  Information-technology Promotion Agency, Japan
 *
 * Author: Shugo Maeda <shugo@ruby-lang.org>
 *
 * This file is part of mod_ruby.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA
 */

#include <stdarg.h>
#ifdef WIN32
#include <windows.h>
#endif

#include "httpd.h"
#include "http_config.h"
#include "http_core.h"
#include "http_log.h"
#include "http_main.h"
#include "http_protocol.h"
#include "http_request.h"
#include "util_script.h"
#include "multithread.h"

#include "ruby.h"
#include "rubyio.h"
#define regoff_t ruby_regoff_t
#define regex_t ruby_regex_t
#define regmatch_t ruby_regmatch_t
#include "re.h"
#include "util.h"
#include "version.h"

#include "mod_ruby.h"
#include "ruby_config.h"
#include "apachelib.h"
#ifdef USE_ERUBY
#include "eruby.h"
#endif
#include "config.h"

extern char **environ;
static char **origenviron;

extern VALUE ruby_errinfo;
extern VALUE rb_defout;
extern VALUE rb_stdin;

extern VALUE rb_load_path;
static VALUE orig_load_path;

#ifdef MULTITHREAD
static mutex *mod_ruby_mutex = NULL;
#endif
static int ruby_is_running = 0;
static int exit_status;

static const command_rec ruby_cmds[] =
{
    {"RubyKanjiCode", ruby_cmd_kanji_code, NULL, OR_ALL, TAKE1,
     "set $KCODE"},
    {"RubyRequire", ruby_cmd_require, NULL, OR_ALL, ITERATE,
     "ruby script name, pulled in via require"},
    {"RubyPassEnv", ruby_cmd_pass_env, NULL, RSRC_CONF, ITERATE,
     "pass environment variables to ENV"},
    {"RubySetEnv", ruby_cmd_set_env, NULL, OR_ALL, TAKE2,
     "Ruby ENV key and value" },
    {"RubyTimeOut", ruby_cmd_timeout, NULL, RSRC_CONF, TAKE1,
     "time to wait execution of ruby script"},
    {NULL}
};

static int ruby_handler(request_rec*);
#ifdef USE_ERUBY
static int eruby_handler(request_rec*);
#endif

static const handler_rec ruby_handlers[] =
{
    {"ruby-script", ruby_handler},
#ifdef USE_ERUBY
    {ERUBY_MIME_TYPE, eruby_handler},
#endif
    {NULL}
};

static void ruby_startup(server_rec*, pool*);

MODULE_VAR_EXPORT module ruby_module =
{
    STANDARD_MODULE_STUFF,
    ruby_startup,		/* initializer */
    ruby_create_dir_config,	/* dir config creater */
    ruby_merge_dir_config,	/* dir merger --- default is to override */
    ruby_create_server_config,	/* create per-server config structure */
    NULL,			/* merge server config */
    ruby_cmds,			/* command table */
    ruby_handlers,		/* handlers */
    NULL,			/* filename translation */
    NULL,			/* check_user_id */
    NULL,			/* check auth */
    NULL,			/* check access */
    NULL,			/* type_checker */
    NULL,			/* fixups */
    NULL,			/* logger */
    NULL,			/* header parser */
    NULL,			/* child_init */
    NULL,			/* child_exit */
    NULL,			/* post read-request */
#ifdef EAPI
    NULL,			/* add_module */
    NULL,			/* remove_module */
    NULL,			/* rewrite_command */
    NULL,			/* new_connection */
    NULL			/* close_connection */
#endif /* EAPI */
};

/* copied from eval.c */
#define TAG_RETURN	0x1
#define TAG_BREAK	0x2
#define TAG_NEXT	0x3
#define TAG_RETRY	0x4
#define TAG_REDO	0x5
#define TAG_RAISE	0x6
#define TAG_THROW	0x7
#define TAG_FATAL	0x8
#define TAG_MASK	0xf

#define STRING_LITERAL(s) rb_str_new(s, sizeof(s) - 1)
#define STR_CAT_LITERAL(str, s) rb_str_cat(str, s, sizeof(s) - 1)

struct pcall_arg {
    VALUE recv;
    ID mid;
    int argc;
    VALUE *argv;
};

static VALUE protect_funcall0(struct pcall_arg *arg)
{
    return rb_funcall2(arg->recv, arg->mid, arg->argc, arg->argv);
}

static VALUE protect_funcall(VALUE recv, ID mid, int *state, int argc, ...)
{
    va_list ap;
    VALUE *argv;
    struct pcall_arg arg;

    if (argc > 0) {
	int i;

	argv = ALLOCA_N(VALUE, argc);

	va_start(ap, argc);
	for (i = 0; i < argc; i++) {
	    argv[i] = va_arg(ap, VALUE);
	}
	va_end(ap);
    }
    else {
	argv = 0;
    }
    arg.recv = recv;
    arg.mid = mid;
    arg.argc = argc;
    arg.argv = argv;
    return rb_protect(protect_funcall0, (VALUE) &arg, state);
}

int ruby_running()
{
    return ruby_is_running;
}

int ruby_require(char *filename)
{
    int state;

    rb_protect(rb_require, (VALUE) filename, &state);
    return state;
}

static VALUE f_p(int argc, VALUE *argv, VALUE self)
{
    int i;

    for (i = 0; i < argc; i++) {
	rb_io_write(rb_defout, rb_inspect(argv[i]));
	rb_io_write(rb_defout, rb_default_rs);
    }
    return Qnil;
}

static VALUE f_exit(int argc, VALUE *argv, VALUE obj)
{
    VALUE status;

    rb_secure(4);
    if (rb_scan_args(argc, argv, "01", &status) == 1) {
	exit_status = NUM2INT(status);
	if (exit_status < 0)
	    rb_raise(rb_eArgError, "negative status code %d", exit_status);
    }
    else {
	exit_status = OK;
    }
    rb_exc_raise(rb_exc_new(rb_eSystemExit, 0, 0));
    return Qnil;		/* not reached */
}

static void ruby_startup(server_rec *s, pool *p)
{
    ruby_server_config *conf =
	(ruby_server_config *) ap_get_module_config(s->module_config,
						    &ruby_module);
    static char ruby_version[BUFSIZ];
    char **list;
    int i;

#if MODULE_MAGIC_NUMBER >= 19980507
    ap_add_version_component(MOD_RUBY_STRING_VERSION);
    snprintf(ruby_version, BUFSIZ, "Ruby/%s(%s)", RUBY_VERSION, RUBY_RELEASE_DATE);
    ap_add_version_component(ruby_version);
#endif

#ifdef MULTITHREAD
    mod_ruby_mutex = ap_create_mutex("mod_ruby_mutex");
#endif

    ruby_init();
    rb_define_global_function("p", f_p, -1);
    rb_define_global_function("exit", f_exit, -1);
    ruby_init_apachelib();
#ifdef USE_ERUBY
    eruby_init();
#endif

    rb_define_global_const("MOD_RUBY",
			   STRING_LITERAL(MOD_RUBY_STRING_VERSION));

    origenviron = environ;

    list = (char **) conf->required_files->elts;
    for (i = 0; i < conf->required_files->nelts; i++) {
	if (ruby_require(list[i])) {
	    fprintf(stderr, "Require of Ruby file `%s' failed, exiting...\n", 
		    list[i]);
	    exit(1);
	}
    }

#if RUBY_VERSION_CODE >= 160
    ruby_init_loadpath();
#else
#if RUBY_VERSION_CODE >= 145
    rb_ary_push(rb_load_path, rb_str_new2("."));
#endif
#endif
    orig_load_path = rb_load_path;
    rb_global_variable(&orig_load_path);

    rb_set_safe_level(1);

    ruby_is_running = 1;
}

static void mr_clearenv()
{
#ifdef WIN32
    char *orgp, *p;

    orgp = p = GetEnvironmentStrings();

    if (p == NULL)
	return;

    while (*p) {
	char buf[1024];
	char *q;

	strncpy(buf, p, sizeof buf);
	q = strchr(buf, '=');
	if (q)
	    *(q+1) = '\0';

	putenv(buf);
	p += strlen(p) + 1;
    }

    FreeEnvironmentStrings(orgp);
#else
    if (environ == origenviron) {
	environ = ALLOC_N(char*, 1);
    }
    else {
	char **p;

	for (p = environ; *p; p++) {
	    if (*p) free(*p);
	}
	REALLOC_N(environ, char*, 1);
    }
    *environ = NULL;
#endif
}

static void mr_setenv(const char *name, const char *value)
{
    if (!name) return;
    if (value && *value)
	ruby_setenv(name, value);
    else
	ruby_unsetenv(name);
}

static void setenv_from_table(table *tbl)
{
    array_header *env_arr;
    table_entry *env;
    int i;

    env_arr = ap_table_elts(tbl);
    env = (table_entry *) env_arr->elts;
    for (i = 0; i < env_arr->nelts; i++) {
	if (env[i].key == NULL)
	    continue;
	mr_setenv(env[i].key, env[i].val);
    }
}

static void setup_env(request_rec *r, ruby_dir_config *dconf)
{
    ruby_server_config *sconf =
	(ruby_server_config *) ap_get_module_config(r->server->module_config,
						    &ruby_module);

    mr_clearenv();
    ap_add_common_vars(r);
    ap_add_cgi_vars(r);
    setenv_from_table(r->subprocess_env);
    setenv_from_table(sconf->env);
    if (dconf) setenv_from_table(dconf->env);
    mr_setenv("MOD_RUBY", MOD_RUBY_STRING_VERSION);
    mr_setenv("GATEWAY_INTERFACE", RUBY_GATEWAY_INTERFACE);
}

static void get_error_pos(VALUE str)
{
    char buff[BUFSIZ];
    ID last_func = rb_frame_last_func();

    if (ruby_sourcefile) {
	if (last_func) {
	    snprintf(buff, BUFSIZ, "%s:%d:in `%s'", ruby_sourcefile, ruby_sourceline,
		     rb_id2name(last_func));
	}
	else {
	    snprintf(buff, BUFSIZ, "%s:%d", ruby_sourcefile, ruby_sourceline);
	}
	rb_str_cat(str, buff, strlen(buff));
    }
}

static void get_exception_info(VALUE str)
{
    VALUE errat;
    VALUE eclass;
    VALUE estr;
    char *einfo;
    int elen;
    int state;

    if (NIL_P(ruby_errinfo)) return;

    errat = rb_funcall(ruby_errinfo, rb_intern("backtrace"), 0);
    if (!NIL_P(errat)) {
	VALUE mesg = RARRAY(errat)->ptr[0];

	if (NIL_P(mesg)) {
	    get_error_pos(str);
	}
	else {
	    rb_str_cat(str, RSTRING(mesg)->ptr, RSTRING(mesg)->len);
	}
    }

    eclass = CLASS_OF(ruby_errinfo);
    estr = rb_protect(rb_obj_as_string, ruby_errinfo, &state);
    if (state) {
	einfo = "";
	elen = 0;
    }
    else {
	einfo = str2cstr(estr, &elen);
    }
    if (eclass == rb_eRuntimeError && elen == 0) {
	STR_CAT_LITERAL(str, ": unhandled exception\n");
    }
    else {
	VALUE epath;

	epath = rb_class_path(eclass);
	if (elen == 0) {
	    STR_CAT_LITERAL(str, ": ");
	    rb_str_cat(str, RSTRING(epath)->ptr, RSTRING(epath)->len);
	    STR_CAT_LITERAL(str, "\n");
	}
	else {
	    char *tail  = 0;
	    int len = elen;

	    if (RSTRING(epath)->ptr[0] == '#') epath = 0;
	    if ((tail = strchr(einfo, '\n')) != NULL) {
		len = tail - einfo;
		tail++;		/* skip newline */
	    }
	    STR_CAT_LITERAL(str, ": ");
	    rb_str_cat(str, einfo, len);
	    if (epath) {
		STR_CAT_LITERAL(str, " (");
		rb_str_cat(str, RSTRING(epath)->ptr, RSTRING(epath)->len);
		STR_CAT_LITERAL(str, ")\n");
	    }
	    if (tail) {
		rb_str_cat(str, tail, elen - len - 1);
		STR_CAT_LITERAL(str, "\n");
	    }
	}
    }

    if (!NIL_P(errat)) {
	int i;
	struct RArray *ep = RARRAY(errat);

#define TRACE_MAX (TRACE_HEAD+TRACE_TAIL+5)
#define TRACE_HEAD 8
#define TRACE_TAIL 5

	rb_ary_pop(errat);
	ep = RARRAY(errat);
	for (i=1; i<ep->len; i++) {
	    if (TYPE(ep->ptr[i]) == T_STRING) {
		STR_CAT_LITERAL(str, "\tfrom ");
		rb_str_cat(str, RSTRING(ep->ptr[i])->ptr, RSTRING(ep->ptr[i])->len);
		STR_CAT_LITERAL(str, "\n");
	    }
	    if (i == TRACE_HEAD && ep->len > TRACE_MAX) {
		char buff[BUFSIZ];
		snprintf(buff, BUFSIZ, "\t ... %ld levels...\n",
			 ep->len - TRACE_HEAD - TRACE_TAIL);
		rb_str_cat(str, buff, strlen(buff));
		i = ep->len - TRACE_TAIL;
	    }
	}
    }
    ruby_errinfo = Qnil;
}

static void ruby_error_print(request_rec *r, int state)
{
    char buff[BUFSIZ];
    VALUE errmsg, logmsg;

    r->content_type = "text/html";
    ap_send_http_header(r);
    ap_rputs("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\">\n", r);
    ap_rputs("<html>\n", r);
    ap_rputs("<head><title>Error</title></head>\n", r);
    ap_rputs("<body>\n", r);
    ap_rputs("<pre>\n", r);

#if 0
    {
	int pid;
	pid = getpid();
	ap_rprintf(r, "pid: %d\n", pid);
	ap_log_error(APLOG_MARK, APLOG_ERR | APLOG_NOERRNO, r->server,
		     "pid: %d\n", pid);
    }
#endif

    errmsg = STRING_LITERAL("");
    switch (state) {
    case TAG_RETURN:
	get_error_pos(errmsg);
	STR_CAT_LITERAL(errmsg, ": unexpected return\n");
	break;
    case TAG_NEXT:
	get_error_pos(errmsg);
	STR_CAT_LITERAL(errmsg, ": unexpected next\n");
	break;
    case TAG_BREAK:
	get_error_pos(errmsg);
	STR_CAT_LITERAL(errmsg, ": unexpected break\n");
	break;
    case TAG_REDO:
	get_error_pos(errmsg);
	STR_CAT_LITERAL(errmsg, ": unexpected redo\n");
	break;
    case TAG_RETRY:
	get_error_pos(errmsg);
	STR_CAT_LITERAL(errmsg, ": retry outside of rescue clause\n");
	break;
    case TAG_RAISE:
    case TAG_FATAL:
	get_exception_info(errmsg);
	break;
    default:
	get_error_pos(errmsg);
	snprintf(buff, BUFSIZ, ": unknown longjmp status %d", state);
	rb_str_cat(errmsg, buff, strlen(buff));
	break;
    }
    ap_rputs(ap_escape_html(r->pool, STR2CSTR(errmsg)), r);
    logmsg = STRING_LITERAL("ruby script error\n");
    rb_str_concat(logmsg, errmsg);
    ap_log_error(APLOG_MARK, APLOG_ERR | APLOG_NOERRNO, r->server,
		 "%s", STR2CSTR(logmsg));

    ap_rputs("</pre>\n", r);
    ap_rputs("</body>\n", r);
    ap_rputs("</html>\n", r);
}

static VALUE kill_threads(VALUE arg)
{
    extern VALUE rb_thread_list();
    VALUE threads = rb_thread_list();
    VALUE main_thread = rb_thread_main();
    VALUE th;
    int i;

    for (i = 0; i < RARRAY(threads)->len; i++) {
	th = RARRAY(threads)->ptr[i];
	if (th != main_thread)
	    protect_funcall(th, rb_intern("kill"), NULL, 0);
    }
    return Qnil;
}

static VALUE load_ruby_script(request_rec *r)
{
    int state;
    request_data *data;

    rb_load_protect(rb_str_new2(r->filename), 1, &state);
#if defined(RUBY_RELEASE_CODE) && RUBY_RELEASE_CODE >= 19990601
    rb_exec_end_proc();
#endif
    if (state && !rb_obj_is_kind_of(ruby_errinfo, rb_eSystemExit)) {
	Data_Get_Struct(rb_defout, request_data, data);
	ruby_error_print(r, state);
    }
    else {
	rb_request_flush(rb_defout);
    }
    return Qnil;
}

#ifdef USE_ERUBY
static VALUE load_eruby_script(request_rec *r)
{
    VALUE script;
    int state;
    request_data *data;

    eruby_noheader = 0;
    eruby_charset = eruby_default_charset;
    script = eruby_load(r->filename, 1, &state);
    if (!NIL_P(script)) unlink(STR2CSTR(script));
#if defined(RUBY_RELEASE_CODE) && RUBY_RELEASE_CODE >= 19990601
    rb_exec_end_proc();
#endif
    if (state && !rb_obj_is_kind_of(ruby_errinfo, rb_eSystemExit)) {
	Data_Get_Struct(rb_defout, request_data, data);
	ruby_error_print(r, state);
    }
    else {
	if (!eruby_noheader) {
	    int len = ruby_request_outbuf_length(rb_defout);

	    if (strcmp(r->content_type, "text/html") == 0) {
		r->content_type = ap_psprintf(r->pool,
					      "text/html; charset=%s",
					      ERUBY_CHARSET);
	    }
	    ap_table_set(r->headers_out, "Content-Length",
			 ap_psprintf(r->pool, "%d", len));
	    rb_request_send_http_header(rb_defout);
	}
	rb_request_flush(rb_defout);
    }
    return Qnil;
}
#endif

struct timeout_arg {
    VALUE thread;
    int timeout;
};

static VALUE do_timeout(struct timeout_arg *arg)
{
    char buff[BUFSIZ];
    VALUE err;

    rb_thread_sleep(arg->timeout);
    snprintf(buff, BUFSIZ, "timeout (%d sec)", arg->timeout);
    err = rb_exc_new2(rb_eApacheTimeoutError, buff);
    rb_funcall(arg->thread, rb_intern("raise"), 1, err);
    return Qnil;
}

static int ruby_handler0(VALUE (*load)(request_rec*), request_rec *r)
{
    ruby_server_config *sconf =
	(ruby_server_config *) ap_get_module_config(r->server->module_config,
						    &ruby_module);
    ruby_dir_config *dconf = NULL;
    VALUE load_thread, timeout_thread;
    struct timeout_arg arg;
    int retval;
    const char *kcode_orig = NULL;
    long i;

    (void) ap_acquire_mutex(mod_ruby_mutex);

    if (r->finfo.st_mode == 0)
	return NOT_FOUND;
    if (S_ISDIR(r->finfo.st_mode))
	return FORBIDDEN;

    if(r->per_dir_config) {
	dconf = (ruby_dir_config *) ap_get_module_config(r->per_dir_config,
							 &ruby_module);
	if (dconf->kcode) {
	    kcode_orig = rb_get_kcode();
	    rb_set_kcode(dconf->kcode);
	}
    }

    if ((retval = ap_setup_client_block(r, REQUEST_CHUNKED_ERROR)))
	return retval;

    ap_chdir_file(r->filename);
    setup_env(r, dconf);
    rb_load_path = rb_ary_new();
    for (i = 0; i < RARRAY(orig_load_path)->len; i++) {
	rb_ary_push(rb_load_path, rb_str_dup(RARRAY(orig_load_path)->ptr[i]));
    }
    exit_status = -1;

    rb_defout = ruby_create_request(r);
    rb_gv_set("$stdin", rb_defout);
    rb_gv_set("$stdout", rb_defout);
    ruby_errinfo = Qnil;
    ruby_debug = Qfalse;
    ruby_verbose = Qfalse;

    ap_soft_timeout("load ruby script", r);
    load_thread = rb_thread_create(load, r);
    arg.thread = load_thread;
    arg.timeout = sconf->timeout;
    timeout_thread = rb_thread_create(do_timeout, (void *) &arg);
    protect_funcall(load_thread, rb_intern("join"), NULL, 0);
    rb_protect(kill_threads, Qnil, NULL);
    ap_kill_timeout(r);

    if (kcode_orig) rb_set_kcode(kcode_orig);
    (void) ap_release_mutex(mod_ruby_mutex);

    load_thread = Qnil;
    rb_gc();

    if (exit_status < 0) {
	return OK;
    }
    else {
	return exit_status;
    }
}

static int ruby_handler(request_rec *r)
{
    return ruby_handler0(load_ruby_script, r);
}

#ifdef USE_ERUBY
static int eruby_handler(request_rec *r)
{
    return ruby_handler0(load_eruby_script, r);
}
#endif

/*
 * Local variables:
 * mode: C
 * tab-width: 8
 * End:
 */
