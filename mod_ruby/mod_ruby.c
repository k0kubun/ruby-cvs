/*
 * $Id$
 * Copyright (C) 2000  ZetaBITS, Inc.
 * Copyright (C) 2000  Information-technology Promotion Agency, Japan
 * Copyright (C) 2001  Shugo Maeda <shugo@modruby.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdarg.h>
#include <signal.h>

#include "mod_ruby.h"
#include "ruby_config.h"
#include "apachelib.h"

#if defined(HAVE_DLOPEN) && !defined(USE_DLN_A_OUT) && !defined(_AIX)
/* dynamic load with dlopen() */
# define USE_DLN_DLOPEN
#endif

#ifdef USE_DLN_DLOPEN
# ifdef __NetBSD__
#  include <nlist.h>
#  include <link.h>
# else
#  include <dlfcn.h>
# endif
#endif

#ifdef __hpux
#include <errno.h>
#include "dl.h"
#endif

#ifdef NeXT
#if NS_TARGET_MAJOR < 4
#include <mach-o/rld.h>
#else
#include <mach-o/dyld.h>
#endif
#endif
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#ifndef WIN32
extern char **environ;
static char **origenviron;
#endif /* WIN32 */

EXTERN VALUE ruby_errinfo;
EXTERN VALUE rb_stdin;
EXTERN VALUE rb_stdout;
EXTERN VALUE rb_defout;

static VALUE orig_stdin;
static VALUE orig_stdout;
static VALUE orig_defout;

EXTERN VALUE rb_load_path;
static VALUE default_load_path;

static const char *default_kcode;

#ifdef MULTITHREAD
#error mod_ruby does not support multi-thread yet.
#endif
static int ruby_is_running = 0;
array_header *ruby_required_libraries = NULL;

static int ruby_object_handler(request_rec *r);
static int ruby_trans_handler(request_rec *r);
static int ruby_authen_handler(request_rec *r);
static int ruby_authz_handler(request_rec *r);
static int ruby_access_handler(request_rec *r);
static int ruby_type_handler(request_rec *r);
static int ruby_fixup_handler(request_rec *r);
static int ruby_log_handler(request_rec *r);
#ifndef APACHE2 /* Apache 1.x */
static int ruby_header_parser_handler(request_rec *r);
#endif
static int ruby_post_read_request_handler(request_rec *r);

static const command_rec ruby_cmds[] =
{
    {"RubyKanjiCode", ruby_cmd_kanji_code, NULL, OR_ALL, TAKE1,
     "set $KCODE"},
    {"RubyAddPath", ruby_cmd_add_path, NULL, OR_ALL, ITERATE,
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
    {"RubyOutputMode", ruby_cmd_output_mode, NULL, OR_ALL, TAKE1,
     "set output mode (nosync|sync|syncheader)"},
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
    {"RubyHeaderParserHandler", ruby_cmd_header_parser_handler,
     NULL, OR_ALL, TAKE1,
     "set header parser object"},
    {"RubyPostReadRequestHandler", ruby_cmd_post_read_request_handler,
     NULL, OR_ALL, TAKE1,
     "set post-read-request handler object"},
    {"RubyInitHandler", ruby_cmd_init_handler,
     NULL, OR_ALL, TAKE1,
     "set init handler object"},
    {"RubyCleanupHandler", ruby_cmd_cleanup_handler,
     NULL, OR_ALL, TAKE1,
     "set cleanup handler object"},
    {NULL}
};

#ifdef APACHE2

static int ruby_startup(pool*, pool*, pool*, server_rec*);
static void ruby_child_init(pool*, server_rec*);

static void ruby_register_hooks(pool *p)
{
    ap_hook_post_config(ruby_startup, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_handler(ruby_object_handler, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_translate_name(ruby_trans_handler, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_check_user_id(ruby_authen_handler, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_auth_checker(ruby_authz_handler, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_access_checker(ruby_access_handler, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_type_checker(ruby_type_handler, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_fixups(ruby_fixup_handler, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_log_transaction(ruby_log_handler, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_child_init(ruby_child_init, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_post_read_request(ruby_post_read_request_handler,
			      NULL, NULL, APR_HOOK_MIDDLE);
}

module AP_MODULE_DECLARE_DATA ruby_module =
{
    STANDARD20_MODULE_STUFF,
    ruby_create_dir_config,	/* dir config creater */
    ruby_merge_dir_config,	/* dir merger --- default is to override */
    ruby_create_server_config,	/* create per-server config structure */
    ruby_merge_server_config,	/* merge server config */
    ruby_cmds,			/* command apr_table_t */
    ruby_register_hooks		/* register hooks */
};

#else /* Apache 1.x */

static void ruby_startup(server_rec*, pool*);
static void ruby_child_init(server_rec*, pool*);

static const handler_rec ruby_handlers[] =
{
    {"ruby-object", ruby_object_handler},
    {NULL}
};

MODULE_VAR_EXPORT module ruby_module =
{
    STANDARD_MODULE_STUFF,
    ruby_startup,		/* initializer */
    ruby_create_dir_config,	/* dir config creater */
    ruby_merge_dir_config,	/* dir merger --- default is to override */
    ruby_create_server_config,	/* create per-server config structure */
    ruby_merge_server_config,	/* merge server config */
    ruby_cmds,			/* command table */
    ruby_handlers,		/* handlers */
    ruby_trans_handler,		/* filename translation */
    ruby_authen_handler,	/* check_user_id */
    ruby_authz_handler,		/* check auth */
    ruby_access_handler,	/* check access */
    ruby_type_handler,		/* type_checker */
    ruby_fixup_handler,		/* fixups */
    ruby_log_handler,		/* logger */
    ruby_header_parser_handler,	/* header parser */
    ruby_child_init,		/* child_init */
    NULL,			/* child_exit */
    ruby_post_read_request_handler,	/* post read-request */
#ifdef EAPI
    NULL,			/* add_module */
    NULL,			/* remove_module */
    NULL,			/* rewrite_command */
    NULL,			/* new_connection */
    NULL			/* close_connection */
#endif /* EAPI */
};
#endif

#define STRING_LITERAL(s) rb_str_new(s, sizeof(s) - 1)
#define STR_CAT_LITERAL(str, s) rb_str_cat(str, s, sizeof(s) - 1)

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

VALUE rb_protect_funcall(VALUE recv, ID mid, int *state, int argc, ...)
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
    /* ruby_errinfo = Qnil; */
}

VALUE ruby_get_error_info(int state)
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

void ruby_log_error(server_rec *s, VALUE errmsg)
{
    VALUE logmsg;

    logmsg = STRING_LITERAL("mod_ruby: error in ruby\n");
    rb_str_concat(logmsg, errmsg);
    ap_log_error(APLOG_MARK, APLOG_ERR | APLOG_NOERRNO, APLOG_STATUS(0) s,
		 "%s", STR2CSTR(logmsg));
}

static void handle_error(request_rec *r, int state)
{
    VALUE errmsg, reqobj;

    errmsg = ruby_get_error_info(state);
    reqobj = (VALUE) ap_get_module_config(r->request_config, &ruby_module);
    if (reqobj)
	rb_apache_request_set_error(reqobj, errmsg, ruby_errinfo);
    ruby_log_error(r->server, errmsg);
}

int ruby_running()
{
    return ruby_is_running;
}

static void setup_loadpath(ruby_server_config *sconf,
			   ruby_dir_config *dconf)
{
    int i, n;
    char **paths;

    rb_load_path = rb_ary_new();
    for (i = 0; i < RARRAY(default_load_path)->len; i++) {
	rb_ary_push(rb_load_path, rb_str_dup(RARRAY(default_load_path)->ptr[i]));
    }
    if (sconf && sconf->load_path) {
	paths = (char **) sconf->load_path->elts;
	n = sconf->load_path->nelts;
	for (i = 0; i < n; i++) {
	    rb_ary_push(rb_load_path, rb_str_new2(paths[i]));
	}
    }
    if (dconf && dconf->load_path) {
	paths = (char **) dconf->load_path->elts;
	n = dconf->load_path->nelts;
	for (i = 0; i < n; i++) {
	    rb_ary_push(rb_load_path, rb_str_new2(paths[i]));
	}
    }
}

int ruby_require(char *filename,
		 ruby_server_config *sconf,
		 ruby_dir_config *dconf)
{
    VALUE fname, exit_status;
    int state;

    setup_loadpath(sconf, dconf);
    rb_protect((VALUE (*)(VALUE)) rb_require, (VALUE) filename, &state);
    fname = rb_str_new2(filename);
    rb_protect_funcall(Qnil, rb_intern("require"), &state, 1, fname);
    if (state == TAG_RAISE &&
	rb_obj_is_kind_of(ruby_errinfo, rb_eSystemExit)) {
	exit_status = rb_iv_get(ruby_errinfo, "status");
	exit(NUM2INT(exit_status));
    }
    return state;
}

void ruby_add_path(const char *path)
{
    rb_ary_push(default_load_path, rb_str_new2(path));
}

static void dso_unload(void *handle)
{
#if defined(HPUX) || defined(HPUX10) || defined(HPUX11)
    shl_unload((shl_t) handle);
#elif defined(HAVE_DYLD)
    NSUnLinkModule(handle, FALSE);
#else
    dlclose(handle);
#endif
}

#if MODULE_MAGIC_NUMBER >= MMN_130 && RUBY_VERSION_CODE >= 164
static APR_CLEANUP_RETURN_TYPE ruby_cleanup(void *data)
{
    EXTERN VALUE ruby_dln_librefs;
    int i;

    ruby_finalize();
    for (i = 0; i < RARRAY(ruby_dln_librefs)->len; i++) {
	dso_unload((void *) NUM2LONG(RARRAY(ruby_dln_librefs)->ptr[i]));
    }
    APR_CLEANUP_RETURN_SUCCESS();
}
#endif

#ifdef POSIX_SIGNAL
#define ruby_signal(sig,handle) posix_signal((sig),(handle))
#else
#define ruby_signal(sig,handle) signal((sig),(handle))
#endif

#ifdef APACHE2
static int ruby_startup(pool *p, pool *plog, pool *ptemp, server_rec *s)
#else /* Apache 1.x */
static void ruby_startup(server_rec *s, pool *p)
#endif
{
    VALUE stack_start;
    ruby_server_config *conf = get_server_config(s);
    ruby_library_context *libraries;
    char **list;
    int i, n;
    int state;
#ifdef SIGHUP
    RETSIGTYPE (*sighup_handler)_((int));
#endif
#ifdef SIGQUIT
    RETSIGTYPE (*sigquit_handler)_((int));
#endif
#ifdef SIGTERM
    RETSIGTYPE (*sigterm_handler)_((int));
#endif
    void Init_stack _((VALUE*));

    if (!ruby_running()) {
#ifdef SIGHUP
	sighup_handler = signal(SIGHUP, SIG_DFL);
#endif
#ifdef SIGQUIT
	sigquit_handler = signal(SIGQUIT, SIG_DFL);
#endif
#ifdef SIGTERM
	sigterm_handler = signal(SIGTERM, SIG_DFL);
#endif
	ruby_init();
#ifdef SIGHUP
	if (sighup_handler != SIG_ERR)
	    ruby_signal(SIGHUP, sighup_handler);
#endif
#ifdef SIGQUIT
	if (sigquit_handler != SIG_ERR)
	    ruby_signal(SIGQUIT, sigquit_handler);
#endif
#ifdef SIGTERM
	if (sigterm_handler != SIG_ERR)
	    ruby_signal(SIGTERM, sigterm_handler);
#endif

	Init_stack(&stack_start);
	rb_init_apache();

	rb_define_global_const("MOD_RUBY",
			       STRING_LITERAL(MOD_RUBY_STRING_VERSION));

#ifndef WIN32
	origenviron = environ;
#endif /* WIN32 */

	orig_stdin = rb_stdin;
	orig_stdout = rb_stdout;
	orig_defout = rb_defout;

	ruby_init_loadpath();
	default_load_path = rb_load_path;
	rb_global_variable(&default_load_path);
	list = (char **) conf->load_path->elts;
	n = conf->load_path->nelts;
	for (i = 0; i < n; i++) {
	    ruby_add_path(list[i]);
	}

	default_kcode = rb_get_kcode();

	if (ruby_required_libraries) {
	    libraries = (ruby_library_context *) ruby_required_libraries->elts;
	    n = ruby_required_libraries->nelts;
	    for (i = 0; i < n; i++) {
		if ((state = ruby_require(libraries[i].filename,
					  libraries[i].server_config,
					  libraries[i].dir_config))) {
		    ap_log_error(APLOG_MARK, APLOG_ERR | APLOG_NOERRNO,
				 APLOG_STATUS(0) s,
				 "mod_ruby: failed to require %s",
				 libraries[i].filename);
		    ruby_log_error(s, ruby_get_error_info(state));
		}
	    }
	}

	ruby_is_running = 1;
    }

#if MODULE_MAGIC_NUMBER >= 19980507
    {
	static char buf[BUFSIZ];
	VALUE v;

	v = rb_const_get(rb_cObject, rb_intern("RUBY_VERSION"));
	snprintf(buf, BUFSIZ, "Ruby/%s", STR2CSTR(v));
#ifdef APACHE2
	ap_add_version_component(p, MOD_RUBY_STRING_VERSION);
	ap_add_version_component(p, buf);
#else /* Apache 1.x */
	ap_add_version_component(MOD_RUBY_STRING_VERSION);
	ap_add_version_component(buf);
#endif
    }
#endif

#if MODULE_MAGIC_NUMBER >= MMN_130 && RUBY_VERSION_CODE >= 164
    if (ruby_module.dynamic_load_handle) 
	ap_register_cleanup(p, NULL, ruby_cleanup, ap_null_cleanup);
#endif
#ifdef APACHE2
    return OK;
#endif
}

static APR_CLEANUP_RETURN_TYPE ruby_child_cleanup(void *data)
{
    ruby_finalize();
    APR_CLEANUP_RETURN_SUCCESS();
}

#ifdef APACHE2
static void ruby_child_init(pool *p, server_rec *s)
#else /* Apache 1.x */
static void ruby_child_init(server_rec *s, pool *p)
#endif
{
    ap_register_cleanup(p, NULL, ruby_child_cleanup, ap_null_cleanup);
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
    const array_header *env_arr;
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
	    rb_protect_funcall(th, rb_intern("kill"), NULL, 0);
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
    rb_protect_funcall(timeout_thread, rb_intern("kill"), NULL, 0);
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
    ret = rb_protect_funcall(thread, rb_intern("value"), &state, 0);
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
    ruby_server_config *sconf;
    ruby_dir_config *dconf;

    dconf = get_dir_config(r);
    sconf = get_server_config(r->server);
    setup_loadpath(sconf, dconf);
    ruby_debug = Qfalse;
    ruby_verbose = Qfalse;
    if (dconf->kcode)
	rb_set_kcode(dconf->kcode);
    rb_request = rb_get_request_object(r);
    rb_stdin = rb_stdout = rb_defout = rb_request;
}

static VALUE exec_end_proc(VALUE arg)
{
    rb_exec_end_proc();
    return Qnil;
}

static void per_request_cleanup(request_rec *r, int flush)
{
    VALUE reqobj;

    while (r->next)
      r = r->next;

    rb_protect(exec_end_proc, Qnil, NULL);
    if (flush) {
	reqobj = rb_get_request_object(r);
	if (reqobj != Qnil)
	    rb_apache_request_flush(reqobj);
    }
    if (r->main) {
	rb_request = rb_get_request_object(r->main);
	rb_stdin = rb_stdout = rb_defout = rb_request;
    } else {
	rb_request = Qnil;
	rb_stdin = orig_stdin;
	rb_stdout = orig_stdout;
	rb_defout = orig_defout;
    }
    rb_set_kcode(default_kcode);
}

typedef struct handler_arg {
    request_rec *r;
    char *handler;
    ID mid;
} handler_arg_t;

static VALUE ruby_handler_0(void *arg)
{
    handler_arg_t *ha = (handler_arg_t *) arg;
    request_rec *r = ha->r;
    char *handler = ha->handler;
    ID mid = ha->mid;
    VALUE ret;
    int state;

    ret = rb_protect_funcall(rb_eval_string(handler), mid, &state,
			     1, rb_request);
    if (state) {
	if (state == TAG_RAISE &&
	    rb_obj_is_kind_of(ruby_errinfo, rb_eSystemExit)) {
	    ret = rb_iv_get(ruby_errinfo, "status");
	}
	else {
	    handle_error(r, state);
	    return INT2NUM(HTTP_INTERNAL_SERVER_ERROR);
	}
    }
    if (FIXNUM_P(ret)) {
	return ret;
    }
    else {
	ap_log_error(APLOG_MARK, APLOG_ERR | APLOG_NOERRNO,
		     APLOG_STATUS(0) r->server,
		     "mod_ruby: %s.%s: handler should return Integer",
		     handler, rb_id2name(mid));
	return INT2NUM(HTTP_INTERNAL_SERVER_ERROR);
    }
}

static int ruby_handler(request_rec *r,
			array_header *handlers_arr, ID mid,
			int run_all, int flush)
{
    ruby_server_config *sconf;
    ruby_dir_config *dconf;
    int safe_level;
    int retval;
    int state;
    VALUE ret;
    handler_arg_t arg;
    int i, handlers_len;
    char **handlers;

    if (handlers_arr == NULL)
	return DECLINED;

    sconf = get_server_config(r->server);
    dconf = get_dir_config(r);
    safe_level = dconf->safe_level;
    handlers = (char **) handlers_arr->elts;
    handlers_len = handlers_arr->nelts;
    retval = DECLINED;

    per_request_init(r);
    for (i = 0; i < handlers_len; i++) {
	arg.r = r;
	arg.handler = handlers[i];
	arg.mid = mid;
	ap_soft_timeout("call ruby handler", r);
	if ((state = run_safely(safe_level, sconf->timeout,
				ruby_handler_0, &arg, &ret)) == 0) {
	    retval = NUM2INT(ret);
	}
	else {
	    handle_error(r, state);
	    retval = HTTP_INTERNAL_SERVER_ERROR;
	}
	ap_kill_timeout(r);
	if (retval != DECLINED && (!run_all || retval != OK))
	    break;
    }
    per_request_cleanup(r, flush && retval == OK);
    return retval;
}

static int ruby_object_handler(request_rec *r)
{
    ruby_dir_config *dconf = get_dir_config(r);
    
    return ruby_handler(r, dconf->ruby_handler, rb_intern("handler"), 0, 1);
}

static int ruby_trans_handler(request_rec *r)
{
    ruby_dir_config *dconf = get_dir_config(r);

    return ruby_handler(r, dconf->ruby_trans_handler,
			rb_intern("translate_uri"), 0, 0);
}

static int ruby_authen_handler(request_rec *r)
{
    ruby_dir_config *dconf = get_dir_config(r);
    int retval;

    if (dconf->ruby_authen_handler == NULL) return DECLINED;
#if 0
    ap_table_set(r->notes, "ruby_in_authen_handler", "true");
#endif
    retval = ruby_handler(r, dconf->ruby_authen_handler,
			  rb_intern("authenticate"), 0, 0);
#if 0
    ap_table_unset(r->notes, "ruby_in_authen_handler");
#endif
    return retval;
}

static int ruby_authz_handler(request_rec *r)
{
    ruby_dir_config *dconf = get_dir_config(r);

    return ruby_handler(r, dconf->ruby_authz_handler,
			rb_intern("authorize"), 0, 0);
}

static int ruby_access_handler(request_rec *r)
{
    ruby_dir_config *dconf = get_dir_config(r);

    return ruby_handler(r, dconf->ruby_access_handler,
			rb_intern("check_access"), 1, 0);
}

static int ruby_type_handler(request_rec *r)
{
    ruby_dir_config *dconf = get_dir_config(r);

    if (dconf->ruby_type_handler == NULL) return DECLINED;
    return ruby_handler(r, dconf->ruby_type_handler,
			rb_intern("find_types"), 0, 0);
}

static int ruby_fixup_handler(request_rec *r)
{
    ruby_dir_config *dconf = get_dir_config(r);

    if (dconf->ruby_fixup_handler == NULL) return DECLINED;
    return ruby_handler(r, dconf->ruby_fixup_handler,
			rb_intern("fixup"), 1, 0);
}

static int ruby_log_handler(request_rec *r)
{
    ruby_dir_config *dconf = get_dir_config(r);

    if (dconf->ruby_log_handler == NULL) return DECLINED;
    return ruby_handler(r, dconf->ruby_log_handler,
			rb_intern("log_transaction"), 1, 0);
}

#ifndef APACHE2 /* Apache 1.x */
static int ruby_header_parser_handler(request_rec *r)
{
    ruby_dir_config *dconf = get_dir_config(r);
    int retval;

    if (dconf->ruby_init_handler &&
	ap_table_get(r->notes, "ruby_init_ran") == NULL) {
	retval = ruby_handler(r, dconf->ruby_init_handler,
			      rb_intern("init"), 1, 0);
	if (retval != OK && retval != DECLINED)
	    return retval;
    }
    if (dconf->ruby_header_parser_handler == NULL) return DECLINED;
    return ruby_handler(r, dconf->ruby_header_parser_handler,
			rb_intern("header_parse"), 1, 0);
}
#endif

static APR_CLEANUP_RETURN_TYPE ruby_cleanup_handler(void *data)
{
    request_rec *r = (request_rec *) data;
    ruby_dir_config *dconf = get_dir_config(r);

    ruby_handler(r, dconf->ruby_cleanup_handler,
		 rb_intern("cleanup"), 1, 0);
    APR_CLEANUP_RETURN_SUCCESS();
}

static int ruby_post_read_request_handler(request_rec *r)
{
    ruby_dir_config *dconf = get_dir_config(r);
    int retval;

    ap_register_cleanup(r->pool, (void *) r, ruby_cleanup_handler, 
			ap_null_cleanup);

    if (dconf->ruby_init_handler) {
	retval = ruby_handler(r, dconf->ruby_init_handler,
			      rb_intern("init"), 1, 0);
	ap_table_set(r->notes, "ruby_init_ran", "true");
	if (retval != OK && retval != DECLINED)
	    return retval;
    }
    return ruby_handler(r, dconf->ruby_post_read_request_handler,
			rb_intern("post_read_request"), 1, 0);
}

/*
 * Local variables:
 * mode: C
 * tab-width: 8
 * End:
 */
