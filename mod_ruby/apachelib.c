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

#include "httpd.h"
#include "http_config.h"
#include "http_core.h"
#include "http_log.h"
#include "http_main.h"
#include "http_protocol.h"
#include "util_script.h"
#include "multithread.h"

#include "ruby.h"
#include "version.h"

#include "apachelib.h"

VALUE rb_mApache;
VALUE rb_eApacheTimeoutError;

VALUE rb_request;

void rb_apache_exit(int status)
{
    VALUE exit;

    exit = rb_exc_new(rb_eSystemExit, 0, 0);
    rb_iv_set(exit, "status", INT2NUM(status));
    rb_exc_raise(exit);
}

static VALUE f_exit(int argc, VALUE *argv, VALUE obj)
{
    VALUE status;
    int status_code;

    rb_secure(4);
    if (rb_scan_args(argc, argv, "01", &status) == 1) {
	status_code = NUM2INT(status);
	if (status_code < 0)
	    rb_raise(rb_eArgError, "negative status code %d", status_code);
    }
    else {
	status_code = OK;
    }
    rb_apache_exit(status_code);
    return Qnil;		/* not reached */
}

static VALUE apache_server_version(VALUE self)
{
    return rb_str_new2(ap_get_server_version());
}

static VALUE apache_add_version_component(VALUE self, VALUE component)
{
    ap_add_version_component(STR2CSTR(component));
    return Qnil;
}

static VALUE apache_server_built(VALUE self)
{
    return rb_str_new2(ap_get_server_built());
}

static VALUE apache_request(VALUE self)
{
    return rb_request;
}

static VALUE apache_unescape_url(VALUE self, VALUE url)
{
    char *buf;

    Check_Type(url, T_STRING);
    buf = ALLOCA_N(char, RSTRING(url)->len + 1);
    memcpy(buf, RSTRING(url)->ptr, RSTRING(url)->len);
    buf[RSTRING(url)->len] = '\0';
    ap_unescape_url(buf);
    return rb_str_new2(buf);
}

static VALUE apache_chdir_file(VALUE self, VALUE file)
{
    ap_chdir_file(STR2CSTR(file));
    return Qnil;
}

void rb_init_apache()
{
    rb_request = Qnil;
    rb_global_variable(&rb_request);

    rb_define_global_function("exit", f_exit, -1);

    rb_mApache = rb_define_module("Apache");
    rb_define_module_function(rb_mApache, "server_version", apache_server_version, 0);
    rb_define_module_function(rb_mApache, "add_version_component",
			      apache_add_version_component, 1);
    rb_define_module_function(rb_mApache, "server_built", apache_server_built, 0);
    rb_define_module_function(rb_mApache, "request", apache_request, 0);
    rb_define_module_function(rb_mApache, "unescape_url", apache_unescape_url, 1);
    rb_define_module_function(rb_mApache, "chdir_file", apache_chdir_file, 1);

    rb_eApacheTimeoutError =
	rb_define_class_under(rb_mApache, "TimeoutError", rb_eException);

    rb_define_const(rb_mApache, "DECLINED", INT2NUM(DECLINED));
    rb_define_const(rb_mApache, "DONE", INT2NUM(DONE));
    rb_define_const(rb_mApache, "OK", INT2NUM(OK));

    /* HTTP status codes */
    rb_define_const(rb_mApache, "HTTP_CONTINUE",
		    INT2NUM(HTTP_CONTINUE));
    rb_define_const(rb_mApache, "HTTP_SWITCHING_PROTOCOLS",
		    INT2NUM(HTTP_SWITCHING_PROTOCOLS));
    rb_define_const(rb_mApache, "HTTP_PROCESSING",
		    INT2NUM(HTTP_PROCESSING));
    rb_define_const(rb_mApache, "HTTP_OK",
		    INT2NUM(HTTP_OK));
    rb_define_const(rb_mApache, "HTTP_CREATED",
		    INT2NUM(HTTP_CREATED));
    rb_define_const(rb_mApache, "HTTP_ACCEPTED",
		    INT2NUM(HTTP_ACCEPTED));
    rb_define_const(rb_mApache, "HTTP_NON_AUTHORITATIVE",
		    INT2NUM(HTTP_NON_AUTHORITATIVE));
    rb_define_const(rb_mApache, "HTTP_NO_CONTENT",
		    INT2NUM(HTTP_NO_CONTENT));
    rb_define_const(rb_mApache, "HTTP_RESET_CONTENT",
		    INT2NUM(HTTP_RESET_CONTENT));
    rb_define_const(rb_mApache, "HTTP_PARTIAL_CONTENT",
		    INT2NUM(HTTP_PARTIAL_CONTENT));
    rb_define_const(rb_mApache, "HTTP_MULTI_STATUS",
		    INT2NUM(HTTP_MULTI_STATUS));
    rb_define_const(rb_mApache, "HTTP_MULTIPLE_CHOICES",
		    INT2NUM(HTTP_MULTIPLE_CHOICES));
    rb_define_const(rb_mApache, "HTTP_MOVED_PERMANENTLY",
		    INT2NUM(HTTP_MOVED_PERMANENTLY));
    rb_define_const(rb_mApache, "HTTP_MOVED_TEMPORARILY",
		    INT2NUM(HTTP_MOVED_TEMPORARILY));
    rb_define_const(rb_mApache, "HTTP_SEE_OTHER",
		    INT2NUM(HTTP_SEE_OTHER));
    rb_define_const(rb_mApache, "HTTP_NOT_MODIFIED",
		    INT2NUM(HTTP_NOT_MODIFIED));
    rb_define_const(rb_mApache, "HTTP_USE_PROXY",
		    INT2NUM(HTTP_USE_PROXY));
    rb_define_const(rb_mApache, "HTTP_TEMPORARY_REDIRECT",
		    INT2NUM(HTTP_TEMPORARY_REDIRECT));
    rb_define_const(rb_mApache, "HTTP_BAD_REQUEST",
		    INT2NUM(HTTP_BAD_REQUEST));
    rb_define_const(rb_mApache, "HTTP_UNAUTHORIZED",
		    INT2NUM(HTTP_UNAUTHORIZED));
    rb_define_const(rb_mApache, "HTTP_PAYMENT_REQUIRED",
		    INT2NUM(HTTP_PAYMENT_REQUIRED));
    rb_define_const(rb_mApache, "HTTP_FORBIDDEN",
		    INT2NUM(HTTP_FORBIDDEN));
    rb_define_const(rb_mApache, "HTTP_NOT_FOUND",
		    INT2NUM(HTTP_NOT_FOUND));
    rb_define_const(rb_mApache, "HTTP_METHOD_NOT_ALLOWED",
		    INT2NUM(HTTP_METHOD_NOT_ALLOWED));
    rb_define_const(rb_mApache, "HTTP_NOT_ACCEPTABLE",
		    INT2NUM(HTTP_NOT_ACCEPTABLE));
    rb_define_const(rb_mApache, "HTTP_PROXY_AUTHENTICATION_REQUIRED",
		    INT2NUM(HTTP_PROXY_AUTHENTICATION_REQUIRED));
    rb_define_const(rb_mApache, "HTTP_REQUEST_TIME_OUT",
		    INT2NUM(HTTP_REQUEST_TIME_OUT));
    rb_define_const(rb_mApache, "HTTP_CONFLICT",
		    INT2NUM(HTTP_CONFLICT));
    rb_define_const(rb_mApache, "HTTP_GONE",
		    INT2NUM(HTTP_GONE));
    rb_define_const(rb_mApache, "HTTP_LENGTH_REQUIRED",
		    INT2NUM(HTTP_LENGTH_REQUIRED));
    rb_define_const(rb_mApache, "HTTP_PRECONDITION_FAILED",
		    INT2NUM(HTTP_PRECONDITION_FAILED));
    rb_define_const(rb_mApache, "HTTP_REQUEST_ENTITY_TOO_LARGE",
		    INT2NUM(HTTP_REQUEST_ENTITY_TOO_LARGE));
    rb_define_const(rb_mApache, "HTTP_REQUEST_URI_TOO_LARGE",
		    INT2NUM(HTTP_REQUEST_URI_TOO_LARGE));
    rb_define_const(rb_mApache, "HTTP_UNSUPPORTED_MEDIA_TYPE",
		    INT2NUM(HTTP_UNSUPPORTED_MEDIA_TYPE));
    rb_define_const(rb_mApache, "HTTP_RANGE_NOT_SATISFIABLE",
		    INT2NUM(HTTP_RANGE_NOT_SATISFIABLE));
    rb_define_const(rb_mApache, "HTTP_EXPECTATION_FAILED",
		    INT2NUM(HTTP_EXPECTATION_FAILED));
    rb_define_const(rb_mApache, "HTTP_UNPROCESSABLE_ENTITY",
		    INT2NUM(HTTP_UNPROCESSABLE_ENTITY));
    rb_define_const(rb_mApache, "HTTP_LOCKED",
		    INT2NUM(HTTP_LOCKED));
#ifdef HTTP_FAILED_DEPENDENCY
    rb_define_const(rb_mApache, "HTTP_FAILED_DEPENDENCY",
			      INT2NUM(HTTP_FAILED_DEPENDENCY));
#endif
    rb_define_const(rb_mApache, "HTTP_INTERNAL_SERVER_ERROR",
		    INT2NUM(HTTP_INTERNAL_SERVER_ERROR));
    rb_define_const(rb_mApache, "HTTP_NOT_IMPLEMENTED",
		    INT2NUM(HTTP_NOT_IMPLEMENTED));
    rb_define_const(rb_mApache, "HTTP_BAD_GATEWAY",
		    INT2NUM(HTTP_BAD_GATEWAY));
    rb_define_const(rb_mApache, "HTTP_SERVICE_UNAVAILABLE",
		    INT2NUM(HTTP_SERVICE_UNAVAILABLE));
    rb_define_const(rb_mApache, "HTTP_GATEWAY_TIME_OUT",
		    INT2NUM(HTTP_GATEWAY_TIME_OUT));
    rb_define_const(rb_mApache, "HTTP_VERSION_NOT_SUPPORTED",
		    INT2NUM(HTTP_VERSION_NOT_SUPPORTED));
    rb_define_const(rb_mApache, "HTTP_VARIANT_ALSO_VARIES",
		    INT2NUM(HTTP_VARIANT_ALSO_VARIES));
#ifdef HTTP_INSUFFICIENT_STORAGE
    rb_define_const(rb_mApache, "HTTP_INSUFFICIENT_STORAGE",
			      INT2NUM(HTTP_INSUFFICIENT_STORAGE));
#endif
    rb_define_const(rb_mApache, "HTTP_NOT_EXTENDED",
		    INT2NUM(HTTP_NOT_EXTENDED));

    rb_define_const(rb_mApache, "DOCUMENT_FOLLOWS",
		    INT2NUM(DOCUMENT_FOLLOWS));
    rb_define_const(rb_mApache, "PARTIAL_CONTENT",
		    INT2NUM(PARTIAL_CONTENT));
    rb_define_const(rb_mApache, "MULTIPLE_CHOICES",
		    INT2NUM(MULTIPLE_CHOICES));
    rb_define_const(rb_mApache, "MOVED",
		    INT2NUM(MOVED));
    rb_define_const(rb_mApache, "REDIRECT",
		    INT2NUM(REDIRECT));
    rb_define_const(rb_mApache, "USE_LOCAL_COPY",
		    INT2NUM(USE_LOCAL_COPY));
    rb_define_const(rb_mApache, "BAD_REQUEST",
		    INT2NUM(BAD_REQUEST));
    rb_define_const(rb_mApache, "AUTH_REQUIRED",
		    INT2NUM(AUTH_REQUIRED));
    rb_define_const(rb_mApache, "FORBIDDEN",
		    INT2NUM(FORBIDDEN));
    rb_define_const(rb_mApache, "NOT_FOUND",
		    INT2NUM(NOT_FOUND));
    rb_define_const(rb_mApache, "METHOD_NOT_ALLOWED",
		    INT2NUM(METHOD_NOT_ALLOWED));
    rb_define_const(rb_mApache, "NOT_ACCEPTABLE",
		    INT2NUM(NOT_ACCEPTABLE));
    rb_define_const(rb_mApache, "LENGTH_REQUIRED",
		    INT2NUM(LENGTH_REQUIRED));
    rb_define_const(rb_mApache, "PRECONDITION_FAILED",
		    INT2NUM(PRECONDITION_FAILED));
    rb_define_const(rb_mApache, "SERVER_ERROR",
		    INT2NUM(SERVER_ERROR));
    rb_define_const(rb_mApache, "NOT_IMPLEMENTED",
		    INT2NUM(NOT_IMPLEMENTED));
    rb_define_const(rb_mApache, "BAD_GATEWAY",
		    INT2NUM(BAD_GATEWAY));
    rb_define_const(rb_mApache, "VARIANT_ALSO_VARIES",
		    INT2NUM(VARIANT_ALSO_VARIES));

    rb_init_apache_array();
    rb_init_apache_table();
    rb_init_apache_connection();
    rb_init_apache_server();
    rb_init_apache_request();
}

/*
 * Local variables:
 * mode: C
 * tab-width: 8
 * End:
 */
