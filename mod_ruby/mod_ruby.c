/*
 * $Id$
 * Copyright (C) 2000  ZetaBITS, Inc.
 * Copyright (C) 2000  Information-technology Promotion Agency, Japan
 * Copyright (C) 2000  Shugo Maeda <shugo@modruby.net>
 *
 * Author: Shugo Maeda <shugo@modruby.net>
 *
 * This file is part of mod_ruby.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
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

#ifndef WIN32
extern char **environ;
static char **origenviron;
#endif /* WIN32 */

EXTERN VALUE ruby_errinfo;
EXTERN VALUE rb_defout;
EXTERN VALUE rb_stdin;
EXTERN VALUE rb_stdout;

EXTERN VALUE rb_load_path;
static VALUE default_load_path;

static const char *default_kcode;

#ifdef MULTITHREAD
static mutex *mod_ruby_mutex = NULL;
#endif
static int ruby_is_running = 0;
/* static int exit_status; */

static const command_rec ruby_cmds[] =
{
    {"RubyKanjiCode", ruby_cmd_kanji_code, NULL, OR_ALL, TAKE1,
     "set $KCODE"},
    {"RubyAddPath", ruby_cmd_add_path, NULL, RSRC_CONF, ITERATE,
     "add path to $:"},
    {"RubyRequire", ruby_cmd_require, NULL, OR_ALL, ITERATE,
     "ruby script name, pulled in via require"},
    {"RubyPassEnv", ruby_cmd_pass_env, NULL, RSRC_CONF, ITERATE,
     "pass environment variables to ENV"},
    {"RubySetEnv", ruby_cmd_set_env, NULL, OR_ALL, TAKE2,
     "Ruby ENV key and value" },
    {"RubyTimeOut", ruby_cmd_timeout, NULL, RSRC_CONF, TAKE1,
     "time to wait execution of ruby script"},
    {"RubySafeLevel", ruby_cmd_safe_level, NULL, OR_ALL, TAKE1,
     "set default $SAFE"},
    {"RubyHandler", ruby_cmd_handler, NULL, OR_ALL, TAKE1,
     "set ruby handler object"},
    {"RubyTransHandler", ruby_cmd_trans_handler, NULL, OR_ALL, TAKE1,
     "set translation handler object"},
    {"RubyAuthenHandler", ruby_cmd_authen_handler, NULL, OR_ALL, TAKE1,
     "set authentication handler object"},
    {"RubyAuthzHandler", ruby_cmd_authz_handler, NULL, OR_ALL, TAKE1,
     "set authorization handler object"},
    {"RubyAccessHandler", ruby_cmd_access_handler, NULL, OR_ALL, TAKE1,
     "set access checker object"},
    {"RubyTypeHandler", ruby_cmd_type_handler, NULL, OR_ALL, TAKE1,
     "set type checker object"},
    {"RubyFixupHandler", ruby_cmd_fixup_handler, NULL, OR_ALL, TAKE1,
     "set fixup handler object"},
    {"RubyLogHandler", ruby_cmd_log_handler, NULL, OR_ALL, TAKE1,
     "set log handler object"},
    {"RubyHeaderParserHandler", ruby_cmd_header_parser_handler, NULL, OR_ALL, TAKE1,
     "set header parser object"},
    {NULL}
};

static int ruby_script_handler(request_rec*);
#ifdef USE_ERUBY
static int eruby_script_handler(request_rec*);
#endif
static int ruby_object_handler(request_rec*);
static int ruby_translate_handler(request_rec*);
static int ruby_authen_handler(request_rec*);
static int ruby_authz_handler(request_rec*);
static int ruby_access_handler(request_rec*);
static int ruby_type_handler(request_rec*);
static int ruby_fixup_handler(request_rec*);
static int ruby_log_handler(request_rec*);
static int ruby_header_parser_handler(request_rec*);

static const handler_rec ruby_handlers[] =
{
    {"ruby-object", ruby_object_handler},
    {"ruby-script", ruby_script_handler},
#ifdef USE_ERUBY
    {ERUBY_MIME_TYPE, eruby_script_handler},
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
    ruby_translate_handler,	/* filename translation */
    ruby_authen_handler,	/* check_user_id */
    ruby_authz_handler,		/* check auth */
    ruby_access_handler,	/* check access */
    ruby_type_handler,		/* type_checker */
    ruby_fixup_handler,		/* fixups */
    ruby_log_handler,		/* logger */
    ruby_header_parser_handler,	/* header parser */
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

#define get_server_config(s) \
	((ruby_server_config *) ap_get_module_config(s->module_config, \
						     &ruby_module))
#define get_dir_config(r) \
	((ruby_dir_config *) ap_get_module_config(r->per_dir_config, \
						  &ruby_module))

typedef struct protect_call_arg {
    VALUE recv;
    ID mid;
    int argc;
    VALUE *argv;
} protect_call_arg_t;

static VALUE protect_funcall0(VALUE arg)
{
    return rb_funcall2(((protect_call_arg_t *) arg)->recv,
		       ((protect_call_arg_t *) arg)->mid,
		       ((protect_call_arg_t *) arg)->argc,
		       ((protect_call_arg_t *) arg)->argv);
}

static VALUE protect_funcall(VALUE recv, ID mid, int *state, int argc, ...)
{
    va_list ap;
    VALUE *argv;
    struct protect_call_arg arg;

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

    rb_protect((VALUE (*)(VALUE)) rb_require, (VALUE) filename, &state);
    return state;
}

void ruby_add_path(const char *path)
{
    rb_ary_push(default_load_path, rb_str_new2(path));
}

#if MODULE_MAGIC_NUMBER >= MMN_130 && RUBY_VERSION_CODE >= 164
static void mod_ruby_dso_unload(void *data)
{
    EXTERN VALUE ruby_dln_librefs;
    int i;

    for (i = 0; i < RARRAY(ruby_dln_librefs)->len; i++) {
	ap_os_dso_unload((void *) NUM2LONG(RARRAY(ruby_dln_librefs)->ptr[i]));
    }
}
#endif

static void ruby_startup(server_rec *s, pool *p)
{
    ruby_server_config *conf = get_server_config(s);
    char **list;
    int i, n;

#ifdef MULTITHREAD
    mod_ruby_mutex = ap_create_mutex("mod_ruby_mutex");
#endif

    if (!ruby_running()) {
	ruby_init();
	rb_init_apache();
#ifdef USE_ERUBY
	eruby_init();
#endif

	rb_define_global_const("MOD_RUBY",
			       STRING_LITERAL(MOD_RUBY_STRING_VERSION));

#ifndef WIN32
	origenviron = environ;
#endif /* WIN32 */

	ruby_init_loadpath();
	default_load_path = rb_load_path;
	rb_global_variable(&default_load_path);
	list = (char **) conf->load_path->elts;
	n = conf->load_path->nelts;
	for (i = 0; i < n; i++) {
	    ruby_add_path(list[i]);
	}

	default_kcode = rb_get_kcode();

	list = (char **) conf->required_files->elts;
	n = conf->required_files->nelts;
	for (i = 0; i < n; i++) {
	    if (ruby_require(list[i])) {
		fprintf(stderr, "Require of Ruby file `%s' failed, exiting...\n", 
			list[i]);
		exit(1);
	    }
	}

	ruby_is_running = 1;
    }

#if MODULE_MAGIC_NUMBER >= 19980507
    {
	static char buf[BUFSIZ];
	VALUE v, d;

	ap_add_version_component(MOD_RUBY_STRING_VERSION);
	v = rb_const_get(rb_cObject, rb_intern("RUBY_VERSION"));
	d = rb_const_get(rb_cObject, rb_intern("RUBY_RELEASE_DATE"));
	snprintf(buf, BUFSIZ, "Ruby/%s(%s)", STR2CSTR(v), STR2CSTR(d));
	ap_add_version_component(buf);
#ifdef USE_ERUBY
	snprintf(buf, BUFSIZ, "eRuby/%s", eruby_version());
	ap_add_version_component(buf);
#endif
    }
#endif

#if MODULE_MAGIC_NUMBER >= MMN_130 && RUBY_VERSION_CODE >= 164
    if (ruby_module.dynamic_load_handle) 
	ap_register_cleanup(p, NULL, mod_ruby_dso_unload, ap_null_cleanup);
#endif
}

static void mod_ruby_clearenv()
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
#ifndef __CYGWIN__
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
#endif
}

static void mod_ruby_setenv(const char *name, const char *value)
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
	mod_ruby_setenv(env[i].key, env[i].val);
    }
}

void rb_setup_cgi_env(request_rec *r)
{
    ruby_server_config *sconf = get_server_config(r->server);
    ruby_dir_config *dconf = get_dir_config(r);

    mod_ruby_clearenv();
    ap_add_common_vars(r);
    ap_add_cgi_vars(r);
    setenv_from_table(r->subprocess_env);
    setenv_from_table(sconf->env);
    setenv_from_table(dconf->env);
    mod_ruby_setenv("MOD_RUBY", MOD_RUBY_STRING_VERSION);
    mod_ruby_setenv("GATEWAY_INTERFACE", RUBY_GATEWAY_INTERFACE);
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
	einfo = RSTRING(estr)->ptr;
	elen = RSTRING(estr)->len;
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
	long i, len;
	struct RArray *ep;

#define TRACE_MAX (TRACE_HEAD+TRACE_TAIL+5)
#define TRACE_HEAD 8
#define TRACE_TAIL 5

	ep = RARRAY(errat);
	len = ep->len;
	for (i=1; i<len; i++) {
	    if (TYPE(ep->ptr[i]) == T_STRING) {
		STR_CAT_LITERAL(str, "\tfrom ");
		rb_str_cat(str, RSTRING(ep->ptr[i])->ptr, RSTRING(ep->ptr[i])->len);
		STR_CAT_LITERAL(str, "\n");
	    }
	    if (i == TRACE_HEAD && len > TRACE_MAX) {
		char buff[BUFSIZ];
		snprintf(buff, BUFSIZ, "\t ... %ld levels...\n",
			 len - TRACE_HEAD - TRACE_TAIL);
		rb_str_cat(str, buff, strlen(buff));
		i = len - TRACE_TAIL;
	    }
	}
    }
    ruby_errinfo = Qnil;
}

static VALUE get_error_info(request_rec *r, int state)
{
    char buff[BUFSIZ];
    VALUE errmsg;

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
    return errmsg;
}

static void print_error(request_rec *r, int state)
{
    VALUE errmsg, logmsg;

    r->content_type = "text/html";
    ap_send_http_header(r);
    ap_rputs("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\">\n", r);
    ap_rputs("<html>\n", r);
    ap_rputs("<head><title>Error</title></head>\n", r);
    ap_rputs("<body>\n", r);
    ap_rputs("<pre>\n", r);

    errmsg = get_error_info(r, state);
    ap_rputs(ap_escape_html(r->pool, STR2CSTR(errmsg)), r);
    logmsg = STRING_LITERAL("mod_ruby:\n");
    rb_str_concat(logmsg, errmsg);
    ap_log_error(APLOG_MARK, APLOG_ERR | APLOG_NOERRNO, r->server,
		 "%s", STR2CSTR(logmsg));

    ap_rputs("</pre>\n", r);
    ap_rputs("</body>\n", r);
    ap_rputs("</html>\n", r);
}

static void log_error(request_rec *r, int state)
{
    VALUE errmsg, logmsg;

    errmsg = get_error_info(r, state);
    ap_rputs(ap_escape_html(r->pool, STR2CSTR(errmsg)), r);
    logmsg = STRING_LITERAL("mod_ruby: error in ruby program\n");
    rb_str_concat(logmsg, errmsg);
    ap_log_error(APLOG_MARK, APLOG_ERR | APLOG_NOERRNO, r->server,
		 "%s", STR2CSTR(logmsg));
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

typedef struct timeout_arg {
    VALUE thread;
    int timeout;
} timeout_arg_t;

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

typedef struct run_safely_arg {
    int safe_level;
    int timeout;
    VALUE (*func)(void*);
    void *arg;
} run_safely_arg_t;

static VALUE run_safely_0(void *arg)
{
    run_safely_arg_t *rsarg = (run_safely_arg_t *) arg;
    struct timeout_arg targ;
    VALUE timeout_thread;
    VALUE result;

    rb_set_safe_level(rsarg->safe_level);
    targ.thread = rb_thread_current();
    targ.timeout = rsarg->timeout;
    timeout_thread = rb_thread_create(do_timeout, (void *) &targ);
    result = (*rsarg->func)(rsarg->arg);
    protect_funcall(timeout_thread, rb_intern("kill"), NULL, 0);
    return result;
}

static int run_safely(int safe_level, int timeout,
		      VALUE (*func)(void*), void *arg, VALUE *retval)
{
    VALUE thread, ret;
    run_safely_arg_t rsarg;
    int state;

    rsarg.safe_level = safe_level;
    rsarg.timeout = timeout;
    rsarg.func = func;
    rsarg.arg = arg;
#if defined(HAVE_SETITIMER)
    rb_thread_start_timer();
#endif
    thread = rb_thread_create(run_safely_0, &rsarg);
    ret = protect_funcall(thread, rb_intern("value"), &state, 0);
    rb_protect(kill_threads, Qnil, NULL);
#if defined(HAVE_SETITIMER)
    rb_thread_stop_timer();
#endif
    if (retval)
	*retval = ret;
    return state;
}

static void per_request_init(request_rec *r)
{
    ruby_dir_config *dconf = get_dir_config(r);
    int i;

    rb_load_path = rb_ary_new();
    for (i = 0; i < RARRAY(default_load_path)->len; i++) {
	rb_ary_push(rb_load_path, rb_str_dup(RARRAY(default_load_path)->ptr[i]));
    }
    ruby_errinfo = Qnil;
    ruby_debug = Qfalse;
    ruby_verbose = Qfalse;
    if (dconf->kcode)
	rb_set_kcode(dconf->kcode);
    rb_request = rb_apache_request_new(r);
}

static void per_request_cleanup()
{
    rb_request = Qnil;
    rb_set_kcode(default_kcode);
}

static VALUE exec_end_proc(VALUE arg)
{
    rb_exec_end_proc();
    return Qnil;
}

typedef struct handler_arg {
    request_rec *r;
    char *handler;
    ID mid;
} handler_arg_t;

static VALUE ruby_request_handler_0(void *arg)
{
    handler_arg_t *ha = (handler_arg_t *) arg;
    request_rec *r = ha->r;
    char *handler = ha->handler;
    ID mid = ha->mid;
    int retval;
    VALUE ret;
    int state;

    ret = protect_funcall(rb_eval_string(handler), mid, &state,
			  1, rb_request);
    if (state) {
	log_error(r, state);
	return INT2NUM(SERVER_ERROR);
    }
    if (TYPE(ret) != T_FIXNUM &&
	TYPE(ret) != T_BIGNUM) {
	ap_log_error(APLOG_MARK, APLOG_ERR | APLOG_NOERRNO, r->server,
		     "mod_ruby: %s.%s: handler should return Integer",
		     handler, rb_id2name(mid));
	return INT2NUM(SERVER_ERROR);
    }
    retval = NUM2INT(ret);
    return INT2NUM(retval);
}

static int ruby_request_handler(request_rec *r, char *handler, ID mid)
{
    ruby_server_config *sconf = get_server_config(r->server);
    ruby_dir_config *dconf = get_dir_config(r);
    int safe_level = dconf->safe_level;
    int retval;
    int state;
    VALUE ret;
    handler_arg_t arg;

    (void) ap_acquire_mutex(mod_ruby_mutex);
    per_request_init(r);
    ap_soft_timeout("call ruby handler", r);
    arg.r = r;
    arg.handler = handler;
    arg.mid = mid;
    if ((state = run_safely(safe_level, sconf->timeout,
			    ruby_request_handler_0, &arg, &ret)) == 0) {
	rb_apache_request_flush(rb_request);
	retval = NUM2INT(ret);
    }
    else {
	log_error(r, state);
	retval = SERVER_ERROR;
    }
    rb_protect(exec_end_proc, Qnil, NULL);
    ap_kill_timeout(r);
    per_request_cleanup();
    (void) ap_release_mutex(mod_ruby_mutex);
    return retval;
}

static int ruby_object_handler(request_rec *r)
{
    ruby_dir_config *dconf = get_dir_config(r);

    if (dconf->ruby_handler == NULL) return DECLINED;
    return ruby_request_handler(r, dconf->ruby_handler,
				rb_intern("handler"));
}

static int ruby_translate_handler(request_rec *r)
{
    ruby_dir_config *dconf = get_dir_config(r);

    if (dconf->ruby_trans_handler == NULL) return DECLINED;
    return ruby_request_handler(r, dconf->ruby_trans_handler,
				rb_intern("translate_name"));
}

static int ruby_authen_handler(request_rec *r)
{
    ruby_dir_config *dconf = get_dir_config(r);

    if (dconf->ruby_authen_handler == NULL) return DECLINED;
    return ruby_request_handler(r, dconf->ruby_authen_handler,
				rb_intern("check_user_id"));
}

static int ruby_authz_handler(request_rec *r)
{
    ruby_dir_config *dconf = get_dir_config(r);

    if (dconf->ruby_authz_handler == NULL) return DECLINED;
    return ruby_request_handler(r, dconf->ruby_authz_handler,
				rb_intern("check_auth"));
}

static int ruby_access_handler(request_rec *r)
{
    ruby_dir_config *dconf = get_dir_config(r);

    if (dconf->ruby_access_handler == NULL) return DECLINED;
    return ruby_request_handler(r, dconf->ruby_access_handler,
				rb_intern("check_access"));
}

static int ruby_type_handler(request_rec *r)
{
    ruby_dir_config *dconf = get_dir_config(r);

    if (dconf->ruby_type_handler == NULL) return DECLINED;
    return ruby_request_handler(r, dconf->ruby_type_handler,
				rb_intern("find_types"));
}

static int ruby_fixup_handler(request_rec *r)
{
    ruby_dir_config *dconf = get_dir_config(r);

    if (dconf->ruby_fixup_handler == NULL) return DECLINED;
    return ruby_request_handler(r, dconf->ruby_fixup_handler,
				rb_intern("fixup"));
}

static int ruby_log_handler(request_rec *r)
{
    ruby_dir_config *dconf = get_dir_config(r);

    if (dconf->ruby_log_handler == NULL) return DECLINED;
    return ruby_request_handler(r, dconf->ruby_log_handler,
				rb_intern("log_transaction"));
}

static int ruby_header_parser_handler(request_rec *r)
{
    ruby_dir_config *dconf = get_dir_config(r);

    if (dconf->ruby_header_parser_handler == NULL) return DECLINED;
    return ruby_request_handler(r, dconf->ruby_header_parser_handler,
				rb_intern("header_parse"));
}

static int script_handler(VALUE (*func)(void*), request_rec *r)
{
    ruby_server_config *sconf = get_server_config(r->server);
    ruby_dir_config *dconf = get_dir_config(r);
    int safe_level = dconf->safe_level;
    VALUE ret;
    VALUE orig_stdin, orig_stdout, orig_defout;
    int retval;

    if (r->finfo.st_mode == 0)
	return NOT_FOUND;
    if (S_ISDIR(r->finfo.st_mode))
	return FORBIDDEN;

    (void) ap_acquire_mutex(mod_ruby_mutex);

    per_request_init(r);
    rb_setup_cgi_env(r);
    orig_stdin = rb_stdin;
    orig_stdout = rb_stdout;
    orig_defout = rb_defout;
    rb_stdin = rb_request;
    rb_stdout = rb_request;
    rb_defout = rb_request;
    if (r->filename)
	ap_chdir_file(r->filename);

    ap_soft_timeout("load ruby script", r);
    if (run_safely(safe_level, sconf->timeout, func, r, &ret) == 0) {
	retval = NUM2INT(ret);
    }
    else {
	retval = SERVER_ERROR;
    }
    ap_kill_timeout(r);

    rb_stdin = orig_stdin;
    rb_stdout = orig_stdout;
    rb_defout = orig_defout;
    per_request_cleanup();

    (void) ap_release_mutex(mod_ruby_mutex);
    return retval;
}

static VALUE load_ruby_script(void *arg)
{
    request_rec *r = (request_rec *) arg;
    int state;
    VALUE ret;

    rb_load_protect(rb_str_new2(r->filename), 1, &state);
    rb_exec_end_proc();
    if (state) {
	if (state == TAG_RAISE &&
	    rb_obj_is_kind_of(ruby_errinfo, rb_eSystemExit)) {
	    ret = rb_iv_get(ruby_errinfo, "status");
	}
	else {
	    print_error(r, state);
	    return INT2NUM(OK);
	}
    }
    else {
	ret = INT2NUM(OK);
    }
    rb_apache_request_flush(rb_request);
    return ret;
}

static int ruby_script_handler(request_rec *r)
{
    return script_handler(load_ruby_script, r);
}

#ifdef USE_ERUBY
static VALUE load_eruby_script(void *arg)
{
    request_rec *r = (request_rec *) arg;
    int state;
    VALUE ret;

    eruby_noheader = 0;
    eruby_charset = eruby_default_charset;
#if defined(ERUBY_VERSION_CODE) && ERUBY_VERSION_CODE >= 90
    eruby_load(r->filename, 0, &state);
#else
    {
	VALUE script;
	script = eruby_load(r->filename, 0, &state);
	if (!NIL_P(script)) unlink(STR2CSTR(script));
    }
#endif
    rb_exec_end_proc();
    if (state) {
	if (state == TAG_RAISE &&
	    rb_obj_is_kind_of(ruby_errinfo, rb_eSystemExit)) {
	    ret = rb_iv_get(ruby_errinfo, "status");
	}
	else {
	    print_error(r, state);
	    return INT2NUM(OK);
	}
    }
    else {
	ret = INT2NUM(OK);
    }
    if (!eruby_noheader) {
	long len = rb_apache_request_length(rb_request);
	
	if (strcmp(r->content_type, "text/html") == 0) {
	    r->content_type = ap_psprintf(r->pool,
					  "text/html; charset=%s",
					  ERUBY_CHARSET);
	}
	ap_set_content_length(r, len);
	rb_apache_request_send_http_header(rb_request);
    }
    rb_apache_request_flush(rb_request);
    return ret;
}

static int eruby_script_handler(request_rec *r)
{
    return script_handler(load_eruby_script, r);
}
#endif

/*
 * Local variables:
 * mode: C
 * tab-width: 8
 * End:
 */
