/*
 * $Id$
 * Copyright (C) 2000  ZetaBITS, Inc.
 * Copyright (C) 2000  Information-technology Promotion Agency, Japan
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

#include <string.h>

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

extern VALUE rb_defout;
extern VALUE rb_stdin;

VALUE rb_mApache;
VALUE rb_cApacheTable;
VALUE rb_cApacheInTable;
VALUE rb_cApacheConnection;
VALUE rb_cApacheServer;
VALUE rb_cApacheRequest;
VALUE rb_eApacheTimeoutError;

#define STRING_LITERAL(s) rb_str_new(s, sizeof(s) - 1)
#define CSTR2OBJ(s) ((s) ? rb_str_new2(s) : Qnil)
#define INT2BOOL(n) ((n) ? Qtrue : Qfalse)

#define DEFINE_ATTR_READER(fname, type, member, convert) \
static VALUE fname(VALUE self) \
{ \
    type *data; \
    Data_Get_Struct(self, type, data); \
    return convert(data->member); \
}

#define DEFINE_ATTR_WRITER(fname, type, member, convert) \
static VALUE fname(VALUE self, VALUE val) \
{ \
    type *data; \
    Data_Get_Struct(self, type, data); \
    data->member = convert(val); \
    return val; \
}

#define DEFINE_STRING_ATTR_READER(fname, type, member) \
	DEFINE_ATTR_READER(fname, type, member, CSTR2OBJ)

#define DEFINE_INT_ATTR_READER(fname, type, member) \
	DEFINE_ATTR_READER(fname, type, member, INT2NUM)
#define DEFINE_INT_ATTR_WRITER(fname, type, member) \
	DEFINE_ATTR_WRITER(fname, type, member, NUM2INT)

#define DEFINE_UINT_ATTR_READER(fname, type, member) \
	DEFINE_ATTR_READER(fname, type, member, UINT2NUM)
#define DEFINE_UINT_ATTR_WRITER(fname, type, member) \
	DEFINE_ATTR_WRITER(fname, type, member, NUM2UINT)

#define DEFINE_LONG_ATTR_READER(fname, type, member) \
	DEFINE_ATTR_READER(fname, type, member, INT2NUM)
#define DEFINE_LONG_ATTR_WRITER(fname, type, member) \
	DEFINE_ATTR_WRITER(fname, type, member, NUM2LONG)

#define DEFINE_ULONG_ATTR_READER(fname, type, member) \
	DEFINE_ATTR_READER(fname, type, member, UINT2NUM)
#define DEFINE_ULONG_ATTR_WRITER(fname, type, member) \
	DEFINE_ATTR_WRITER(fname, type, member, NUM2ULONG)

#define DEFINE_BOOL_ATTR_READER(fname, type, member) \
	DEFINE_ATTR_READER(fname, type, member, INT2BOOL)
#define DEFINE_BOOL_ATTR_WRITER(fname, type, member) \
	DEFINE_ATTR_WRITER(fname, type, member, RTEST)

static VALUE f_p(int argc, VALUE *argv, VALUE self)
{
    int i;

    for (i = 0; i < argc; i++) {
	rb_io_write(rb_defout, rb_inspect(argv[i]));
	rb_io_write(rb_defout, rb_default_rs);
    }
    return Qnil;
}

static void mod_ruby_exit(int status)
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
    mod_ruby_exit(status_code);
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
    return rb_defout;
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

static VALUE table_new(VALUE klass, table *tbl)
{
    return Data_Wrap_Struct(klass, NULL, NULL, tbl);
}

static VALUE table_clear(VALUE self)
{
    table *tbl;

    Data_Get_Struct(self, table, tbl);
    ap_clear_table(tbl);
    return Qnil;
}

static VALUE table_get(VALUE self, VALUE name)
{
    table *tbl;
    const char *key, *res;

    key = STR2CSTR(name);
    Data_Get_Struct(self, table, tbl);
    res = ap_table_get(tbl, key);
    if (res)
	return rb_str_new2(res);
    else
	return Qnil;
}

static VALUE table_set(VALUE self, VALUE name, VALUE val)
{
    table *tbl;

    Data_Get_Struct(self, table, tbl);
    ap_table_set(tbl, STR2CSTR(name), STR2CSTR(val));
    return val;
}

static VALUE table_setn(VALUE self, VALUE name, VALUE val)
{
    table *tbl;

    Data_Get_Struct(self, table, tbl);
    ap_table_setn(tbl, STR2CSTR(name), STR2CSTR(val));
    return val;
}

static VALUE table_merge(VALUE self, VALUE name, VALUE val)
{
    table *tbl;

    Data_Get_Struct(self, table, tbl);
    ap_table_merge(tbl, STR2CSTR(name), STR2CSTR(val));
    return val;
}

static VALUE table_mergen(VALUE self, VALUE name, VALUE val)
{
    table *tbl;

    Data_Get_Struct(self, table, tbl);
    ap_table_mergen(tbl, STR2CSTR(name), STR2CSTR(val));
    return val;
}

static VALUE table_unset(VALUE self, VALUE name)
{
    table *tbl;

    Data_Get_Struct(self, table, tbl);
    ap_table_unset(tbl, STR2CSTR(name));
    return Qnil;
}

static VALUE table_add(VALUE self, VALUE name, VALUE val)
{
    table *tbl;

    Data_Get_Struct(self, table, tbl);
    ap_table_add(tbl, STR2CSTR(name), STR2CSTR(val));
    return val;
}

static VALUE table_addn(VALUE self, VALUE name, VALUE val)
{
    table *tbl;

    Data_Get_Struct(self, table, tbl);
    ap_table_addn(tbl, STR2CSTR(name), STR2CSTR(val));
    return val;
}

static VALUE table_each(VALUE self)
{
    VALUE assoc;
    table *tbl;
    array_header *hdrs_arr;
    table_entry *hdrs;
    int i;

    Data_Get_Struct(self, table, tbl);
    hdrs_arr = ap_table_elts(tbl);
    hdrs = (table_entry *) hdrs_arr->elts;
    for (i = 0; i < hdrs_arr->nelts; i++) {
	if (hdrs[i].key == NULL)
	    continue;
	assoc = rb_assoc_new(rb_str_new2(hdrs[i].key),
			     hdrs[i].val ? rb_str_new2(hdrs[i].val) : Qnil);
	rb_yield(assoc);
    }
    return Qnil;
}

static VALUE table_each_key(VALUE self)
{
    table *tbl;
    array_header *hdrs_arr;
    table_entry *hdrs;
    int i;

    Data_Get_Struct(self, table, tbl);
    hdrs_arr = ap_table_elts(tbl);
    hdrs = (table_entry *) hdrs_arr->elts;
    for (i = 0; i < hdrs_arr->nelts; i++) {
	if (hdrs[i].key == NULL)
	    continue;
	rb_yield(rb_str_new2(hdrs[i].key));
    }
    return Qnil;
}

static VALUE table_each_value(VALUE self)
{
    table *tbl;
    array_header *hdrs_arr;
    table_entry *hdrs;
    int i;

    Data_Get_Struct(self, table, tbl);
    hdrs_arr = ap_table_elts(tbl);
    hdrs = (table_entry *) hdrs_arr->elts;
    for (i = 0; i < hdrs_arr->nelts; i++) {
	if (hdrs[i].key == NULL)
	    continue;
	rb_yield(hdrs[i].val ? rb_str_new2(hdrs[i].val) : Qnil);
    }
    return Qnil;
}

static VALUE in_table_get(VALUE self, VALUE name)
{
    table *tbl;
    const char *key, *res;

    key = STR2CSTR(name);
    if (strcasecmp(key, "authorization") == 0 ||
	strcasecmp(key, "proxy-authorization") == 0)
	return Qnil;
    Data_Get_Struct(self, table, tbl);
    res = ap_table_get(tbl, key);
    if (res)
	return rb_str_new2(res);
    else
	return Qnil;
}

static VALUE in_table_each(VALUE self)
{
    VALUE assoc;
    table *tbl;
    array_header *hdrs_arr;
    table_entry *hdrs;
    int i;

    Data_Get_Struct(self, table, tbl);
    hdrs_arr = ap_table_elts(tbl);
    hdrs = (table_entry *) hdrs_arr->elts;
    for (i = 0; i < hdrs_arr->nelts; i++) {
	if (hdrs[i].key == NULL)
	    continue;
	if (strcasecmp(hdrs[i].key, "authorization") == 0 ||
	    strcasecmp(hdrs[i].key, "proxy-authorization") == 0)
	    continue;
	assoc = rb_assoc_new(rb_str_new2(hdrs[i].key),
			     hdrs[i].val ? rb_str_new2(hdrs[i].val) : Qnil);
	rb_yield(assoc);
    }
    return Qnil;
}

static VALUE in_table_each_key(VALUE self)
{
    table *tbl;
    array_header *hdrs_arr;
    table_entry *hdrs;
    int i;

    Data_Get_Struct(self, table, tbl);
    hdrs_arr = ap_table_elts(tbl);
    hdrs = (table_entry *) hdrs_arr->elts;
    for (i = 0; i < hdrs_arr->nelts; i++) {
	if (hdrs[i].key == NULL)
	    continue;
	if (strcasecmp(hdrs[i].key, "authorization") == 0 ||
	    strcasecmp(hdrs[i].key, "proxy-authorization") == 0)
	    continue;
	rb_yield(rb_str_new2(hdrs[i].key));
    }
    return Qnil;
}

static VALUE in_table_each_value(VALUE self)
{
    table *tbl;
    array_header *hdrs_arr;
    table_entry *hdrs;
    int i;

    Data_Get_Struct(self, table, tbl);
    hdrs_arr = ap_table_elts(tbl);
    hdrs = (table_entry *) hdrs_arr->elts;
    for (i = 0; i < hdrs_arr->nelts; i++) {
	if (hdrs[i].key == NULL)
	    continue;
	if (strcasecmp(hdrs[i].key, "authorization") == 0 ||
	    strcasecmp(hdrs[i].key, "proxy-authorization") == 0)
	    continue;
	rb_yield(hdrs[i].val ? rb_str_new2(hdrs[i].val) : Qnil);
    }
    return Qnil;
}

static VALUE connection_new(conn_rec *conn)
{
    return Data_Wrap_Struct(rb_cApacheConnection, NULL, NULL, conn);
}

DEFINE_STRING_ATTR_READER(connection_remote_ip, conn_rec, remote_ip);
DEFINE_STRING_ATTR_READER(connection_remote_host, conn_rec, remote_host);
DEFINE_STRING_ATTR_READER(connection_remote_logname, conn_rec, remote_logname);
DEFINE_STRING_ATTR_READER(connection_user, conn_rec, user);
DEFINE_STRING_ATTR_READER(connection_auth_type, conn_rec, ap_auth_type);
DEFINE_STRING_ATTR_READER(connection_local_ip, conn_rec, local_ip);
DEFINE_STRING_ATTR_READER(connection_local_host, conn_rec, local_host);

static VALUE server_new(server_rec *server)
{
    return Data_Wrap_Struct(rb_cApacheServer, NULL, NULL, server);
}

DEFINE_STRING_ATTR_READER(server_defn_name, server_rec, defn_name);
DEFINE_UINT_ATTR_READER(server_defn_line_number, server_rec, defn_line_number);
DEFINE_STRING_ATTR_READER(server_srm_confname, server_rec, srm_confname);
DEFINE_STRING_ATTR_READER(server_access_confname, server_rec, access_confname);
DEFINE_STRING_ATTR_READER(server_admin, server_rec, server_admin);
DEFINE_STRING_ATTR_READER(server_hostname, server_rec, server_hostname);
DEFINE_UINT_ATTR_READER(server_port, server_rec, port);
DEFINE_STRING_ATTR_READER(server_error_fname, server_rec, error_fname);
DEFINE_INT_ATTR_READER(server_loglevel, server_rec, loglevel);
DEFINE_BOOL_ATTR_READER(server_is_virtual, server_rec, is_virtual);

typedef struct request_data {
    request_rec *request;
    VALUE outbuf;
    VALUE connection;
    VALUE server;
    VALUE headers_in;
    VALUE headers_out;
    VALUE err_headers_out;
    VALUE subprocess_env;
    VALUE notes;
    VALUE finfo;
    int send_http_header;
    long pos;
} request_data;

#define REQUEST_STRING_ATTR_READER(fname, member) \
	DEFINE_STRING_ATTR_READER(fname, request_data, request->member)

#define REQUEST_STRING_ATTR_WRITER(fname, member) \
static VALUE fname(VALUE self, VALUE val) \
{ \
    request_data *data; \
    Data_Get_Struct(self, request_data, data); \
    data->request->member = \
	ap_pstrdup(data->request->pool, STR2CSTR(val)); \
    return val; \
}

#define REQUEST_INT_ATTR_READER(fname, member) \
	DEFINE_INT_ATTR_READER(fname, request_data, request->member)
#define REQUEST_INT_ATTR_WRITER(fname, member) \
	DEFINE_INT_ATTR_WRITER(fname, request_data, request->member)

static void request_mark(request_data *data)
{
    rb_gc_mark(data->outbuf);
    rb_gc_mark(data->connection);
    rb_gc_mark(data->server);
    rb_gc_mark(data->headers_in);
    rb_gc_mark(data->headers_out);
    rb_gc_mark(data->err_headers_out);
    rb_gc_mark(data->subprocess_env);
    rb_gc_mark(data->notes);
    rb_gc_mark(data->finfo);
}

VALUE ruby_request_new(request_rec *r)
{
    request_data *data;
    VALUE result;
    
    r->content_type = "text/html";
    result = Data_Make_Struct(rb_cApacheRequest, request_data,
			      request_mark, free, data);
    data->request = r;
    data->outbuf = rb_tainted_str_new("", 0);
    data->connection = Qnil;
    data->headers_in = Qnil;
    data->headers_out = Qnil;
    data->err_headers_out = Qnil;
    data->subprocess_env = Qnil;
    data->notes = Qnil;
    data->finfo = Qnil;
    data->send_http_header = 0;
    data->pos = 0;

    return result;
}

long ruby_request_outbuf_length(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    return RSTRING(data->outbuf)->len;
}

static VALUE request_to_s(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    return data->outbuf;
}

static VALUE request_replace(int argc, VALUE *argv, VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    return rb_funcall2(data->outbuf, rb_frame_last_func(), argc, argv);
}

static VALUE request_cancel(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    RSTRING(data->outbuf)->len = 0;
    RSTRING(data->outbuf)->ptr[0] = '\0';
    return Qnil;
}

static VALUE request_write(VALUE self, VALUE str)
{
    request_data *data;
    int len;

    Data_Get_Struct(self, request_data, data);
    str = rb_obj_as_string(str);
    rb_str_cat(data->outbuf, RSTRING(str)->ptr, RSTRING(str)->len);
    len = RSTRING(str)->len;
    return INT2NUM(len);
}

static VALUE request_putc(VALUE self, VALUE c)
{
    char ch = NUM2CHR(c);
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    rb_str_cat(data->outbuf, &ch, 1);
    return c;
}

static VALUE request_print(int argc, VALUE *argv, VALUE out)
{
    int i;
    VALUE line;

    /* if no argument given, print `$_' */
    if (argc == 0) {
	argc = 1;
	line = rb_lastline_get();
	argv = &line;
    }
    for (i=0; i<argc; i++) {
	if (!NIL_P(rb_output_fs) && i>0) {
	    request_write(out, rb_output_fs);
	}
	switch (TYPE(argv[i])) {
	  case T_NIL:
	    request_write(out, STRING_LITERAL("nil"));
	    break;
	  default:
	    request_write(out, argv[i]);
	    break;
	}
    }
    if (!NIL_P(rb_output_rs)) {
	request_write(out, rb_output_rs);
    }

    return Qnil;
}

static VALUE request_printf(int argc, VALUE *argv, VALUE out)
{
    request_write(out, rb_f_sprintf(argc, argv));
    return Qnil;
}

static VALUE request_puts _((int, VALUE*, VALUE));

static VALUE request_puts_ary(VALUE ary, VALUE out)
{
    VALUE tmp;
    int i;

    for (i=0; i<RARRAY(ary)->len; i++) {
	tmp = RARRAY(ary)->ptr[i];
	if (rb_inspecting_p(tmp)) {
	    tmp = STRING_LITERAL("[...]");
	}
	request_puts(1, &tmp, out);
    }
    return Qnil;
}

static VALUE request_puts(int argc, VALUE *argv, VALUE out)
{
    int i;
    VALUE line;

    /* if no argument given, print newline. */
    if (argc == 0) {
	request_write(out, rb_default_rs);
	return Qnil;
    }
    for (i=0; i<argc; i++) {
	switch (TYPE(argv[i])) {
	  case T_NIL:
	    line = STRING_LITERAL("nil");
	    break;
	  case T_ARRAY:
	    rb_protect_inspect(request_puts_ary, argv[i], out);
	    continue;
	  default:
	    line = argv[i];
	    break;
	}
	line = rb_obj_as_string(line);
	request_write(out, line);
	if (RSTRING(line)->ptr[RSTRING(line)->len-1] != '\n') {
	    request_write(out, rb_default_rs);
	}
    }

    return Qnil;
}

static VALUE request_addstr(VALUE out, VALUE str)
{
    request_write(out, str);
    return out;
}

VALUE rb_request_send_http_header(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    data->send_http_header = 1;
    return Qnil;
}

void rb_request_flush(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    if (data->send_http_header) {
	ap_send_http_header(data->request);
	if (data->request->header_only)
	    return;
    }
    if (RSTRING(data->outbuf)->len > 0) {
	ap_rwrite(RSTRING(data->outbuf)->ptr,
		  RSTRING(data->outbuf)->len, data->request);
    }
}


static VALUE request_connection(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    if (NIL_P(data->connection)) {
	data->connection = connection_new(data->request->connection);
    }
    return data->connection;
}

static VALUE request_server(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    if (NIL_P(data->server)) {
	data->server = server_new(data->request->server);
    }
    return data->server;
}

REQUEST_STRING_ATTR_READER(request_protocol, protocol);
REQUEST_STRING_ATTR_READER(request_hostname, hostname);
REQUEST_STRING_ATTR_READER(request_unparsed_uri, unparsed_uri);
REQUEST_STRING_ATTR_READER(request_uri, uri);
REQUEST_STRING_ATTR_READER(request_get_filename, filename);
REQUEST_STRING_ATTR_WRITER(request_set_filename, filename);
REQUEST_STRING_ATTR_READER(request_path_info, path_info);
REQUEST_INT_ATTR_READER(request_get_status, status);
REQUEST_INT_ATTR_WRITER(request_set_status, status);
REQUEST_STRING_ATTR_READER(request_get_status_line, status_line);
REQUEST_STRING_ATTR_WRITER(request_set_status_line, status_line);
REQUEST_STRING_ATTR_READER(request_request_method, method);
REQUEST_STRING_ATTR_READER(request_args, args);
REQUEST_STRING_ATTR_READER(request_get_content_type, content_type);
REQUEST_STRING_ATTR_READER(request_get_content_encoding, content_encoding);

static VALUE request_request_time(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    return rb_time_new(data->request->request_time, 0);
}

static VALUE request_header_only(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    return data->request->header_only ? Qtrue : Qfalse;
}

static VALUE request_content_length(VALUE self)
{
    request_data *data;
    const char *s;

    rb_warn("content_length is obsolete; use headers_in[\"Content-Length\"] instead");
    Data_Get_Struct(self, request_data, data);
    s = ap_table_get(data->request->headers_in, "Content-Length");
    return s ? rb_cstr2inum(s, 10) : Qnil;
}

static VALUE request_set_content_type(VALUE self, VALUE str)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    str = rb_funcall(str, rb_intern("downcase"), 0);
    data->request->content_type =
	ap_pstrdup(data->request->pool, STR2CSTR(str));
    return str;
}

static VALUE request_set_content_encoding(VALUE self, VALUE str)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    str = rb_funcall(str, rb_intern("downcase"), 0);
    data->request->content_encoding =
	ap_pstrdup(data->request->pool, STR2CSTR(str));
    return str;
}

static VALUE request_get_content_languages(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    if (data->request->content_languages) {
	VALUE ary = rb_ary_new();
	int i, len =  data->request->content_languages->nelts;
	char **langs = (char **) data->request->content_languages->elts;
	for (i = 0; i < len; i++) {
	    rb_ary_push(ary, rb_str_new2(langs[i]));
	}
	rb_ary_freeze(ary);
	return ary;
    }
    else {
	return Qnil;
    }
}

static VALUE request_set_content_languages(VALUE self, VALUE ary)
{
    request_data *data;
    int i;

    Check_Type(ary, T_ARRAY);
    for (i = 0; i < RARRAY(ary)->len; i++) {
	Check_Type(RARRAY(ary)->ptr[i], T_STRING);
    }
    Data_Get_Struct(self, request_data, data);
    data->request->content_languages =
	ap_make_array(data->request->pool, RARRAY(ary)->len, sizeof(char *));
    for (i = 0; i < RARRAY(ary)->len; i++) {
	VALUE str = RARRAY(ary)->ptr[i];
	str = rb_funcall(str, rb_intern("downcase"), 0);
        *(char **) ap_push_array(data->request->content_languages) =
	    ap_pstrdup(data->request->pool, STR2CSTR(str));
    }
    return ary;
}

static VALUE request_headers_in(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    if (NIL_P(data->headers_in)) {
	data->headers_in = table_new(rb_cApacheInTable,
				     data->request->headers_in);
    }
    return data->headers_in;
}

static VALUE request_headers_out(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    if (NIL_P(data->headers_out)) {
	data->headers_out = table_new(rb_cApacheTable,
				      data->request->headers_out);
    }
    return data->headers_out;
}

static VALUE request_err_headers_out(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    if (NIL_P(data->err_headers_out)) {
	data->err_headers_out =
	    table_new(rb_cApacheTable, data->request->err_headers_out);
    }
    return data->err_headers_out;
}

static VALUE request_subprocess_env(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    if (NIL_P(data->subprocess_env)) {
	data->subprocess_env = table_new(rb_cApacheTable,
					 data->request->subprocess_env);
    }
    return data->subprocess_env;
}

static VALUE request_notes(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    if (NIL_P(data->notes)) {
	data->notes = table_new(rb_cApacheTable, data->request->notes);
    }
    return data->notes;
}

static VALUE request_finfo(VALUE self)
{
    VALUE cStat, obj;
    request_data *data;
    struct stat *st;

    Data_Get_Struct(self, request_data, data);
    if (NIL_P(data->finfo)) {
	cStat = rb_const_get(rb_cFile, rb_intern("Stat"));
	data->finfo = Data_Make_Struct(cStat, struct stat, NULL, free, st);
	*st = data->request->finfo;
    }
    return data->finfo;
}

static VALUE request_aref(VALUE self, VALUE vkey)
{
    request_data *data;
    char *key = STR2CSTR(vkey);
    const char *val;

    rb_warn("self[] is obsolete; use headers_in instead");
    if (strcasecmp(key, "authorization") == 0 ||
	strcasecmp(key, "proxy-authorization") == 0)
	return Qnil;
    Data_Get_Struct(self, request_data, data);
    val = ap_table_get(data->request->headers_in, key);
    return val ? rb_str_new2(val) : Qnil;
}

static VALUE request_aset(VALUE self, VALUE key, VALUE val)
{
    request_data *data;
    char *s;

    rb_warn("self[]= is obsolete; use headers_out instead");
    s = STR2CSTR(key);
    if (strcasecmp(s, "content-type") == 0) {
	return request_set_content_type(self, val);
    }
    else if (strcasecmp(s, "content-encoding") == 0) {
	return request_set_content_encoding(self, val);
    }
    else {
	Data_Get_Struct(self, request_data, data);
	ap_table_set(data->request->headers_out, s, STR2CSTR(val));
	return val;
    }
}

static VALUE request_each_header(VALUE self)
{
    request_data *data;
    array_header *hdrs_arr;
    table_entry *hdrs;
    int i;
    VALUE assoc;

    rb_warn("each_header is obsolete; use headers_in instead");
    Data_Get_Struct(self, request_data, data);
    hdrs_arr = ap_table_elts(data->request->headers_in);
    hdrs = (table_entry *) hdrs_arr->elts;
    for (i = 0; i < hdrs_arr->nelts; i++) {
	if (hdrs[i].key == NULL)
	    continue;
	if (strcasecmp(hdrs[i].key, "authorization") == 0 ||
	    strcasecmp(hdrs[i].key, "proxy-authorization") == 0)
	    continue;
	assoc = rb_assoc_new(rb_str_new2(hdrs[i].key),
			     hdrs[i].val ? rb_str_new2(hdrs[i].val) : Qnil);
	rb_yield(assoc);
    }
    return Qnil;
}

static VALUE request_each_key(VALUE self)
{
    request_data *data;
    array_header *hdrs_arr;
    table_entry *hdrs;
    int i;

    rb_warn("each_key is obsolete; use headers_in instead");
    Data_Get_Struct(self, request_data, data);
    hdrs_arr = ap_table_elts(data->request->headers_in);
    hdrs = (table_entry *) hdrs_arr->elts;
    for (i = 0; i < hdrs_arr->nelts; i++) {
	if (hdrs[i].key == NULL)
	    continue;
	if (strcasecmp(hdrs[i].key, "authorization") == 0 ||
	    strcasecmp(hdrs[i].key, "proxy-authorization") == 0)
	    continue;
	rb_yield(rb_str_new2(hdrs[i].key));
    }
    return Qnil;
}

static VALUE request_each_value(VALUE self)
{
    request_data *data;
    array_header *hdrs_arr;
    table_entry *hdrs;
    int i;

    rb_warn("each_value is obsolete; use headers_in instead");
    Data_Get_Struct(self, request_data, data);
    hdrs_arr = ap_table_elts(data->request->headers_in);
    hdrs = (table_entry *) hdrs_arr->elts;
    for (i = 0; i < hdrs_arr->nelts; i++) {
	if (hdrs[i].key == NULL)
	    continue;
	if (strcasecmp(hdrs[i].key, "authorization") == 0 ||
	    strcasecmp(hdrs[i].key, "proxy-authorization") == 0)
	    continue;
	rb_yield(hdrs[i].val ? rb_str_new2(hdrs[i].val) : Qnil);
    }
    return Qnil;
}

static VALUE request_setup_client_block(int argc, VALUE *argv, VALUE self)
{
    request_data *data;
    VALUE policy;
    int read_policy = REQUEST_CHUNKED_ERROR;

    Data_Get_Struct(self, request_data, data);
    if (rb_scan_args(argc, argv, "01", &policy) == 1) {
	read_policy = NUM2INT(policy);
    }
    return INT2NUM(ap_setup_client_block(data->request, read_policy));
}

static VALUE request_should_client_block(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    return INT2BOOL(ap_should_client_block(data->request));
}

static VALUE request_get_client_block(VALUE self, VALUE length)
{
    request_data *data;
    char *buf;
    int len;

    Data_Get_Struct(self, request_data, data);
    len = NUM2INT(length);
    buf = (char *) ap_palloc(data->request->pool, len);
    len = ap_get_client_block(data->request, buf, len);
    return rb_tainted_str_new(buf, len);
}

static VALUE read_client_block(request_rec *r, int len)
{
    char *buf;
    int rc;
    int nrd = 0;
    int old_read_length;
    VALUE result;

    if (r->read_length == 0) {
        if ((rc = ap_setup_client_block(r, REQUEST_CHUNKED_ERROR)) != OK) {
	    mod_ruby_exit(rc);
        }
    }
    old_read_length = r->read_length;
    r->read_length = 0;
    if (ap_should_client_block(r)) {
	if (len < 0)
	    len = r->remaining;
	buf = (char *) ap_palloc(r->pool, len);
        nrd = ap_get_client_block(r, buf, len);
	result = rb_tainted_str_new(buf, len);
    }
    else {
	result = Qnil;
    }
    r->read_length += old_read_length;
    return result;
}

static VALUE request_read(int argc, VALUE *argv, VALUE self)
{
    request_data *data;
    VALUE length;
    int len;

    Data_Get_Struct(self, request_data, data);
    rb_scan_args(argc, argv, "01", &length);
    if (NIL_P(length)) {
	return read_client_block(data->request, -1);
    }
    len = NUM2INT(length);
    if (len < 0) {
	rb_raise(rb_eArgError, "negative length %d given", len);
    }
    return read_client_block(data->request, len);
}

static VALUE request_getc(VALUE self)
{
    request_data *data;
    VALUE str;

    Data_Get_Struct(self, request_data, data);
    str = read_client_block(data->request, 1);
    if (NIL_P(str) || RSTRING(str)->len == 0)
	return Qnil;
    return INT2FIX(RSTRING(str)->ptr[0]);
}

static VALUE request_eof(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    return INT2BOOL(data->request->remaining == 0);
}

static VALUE request_binmode(VALUE self)
{
    return Qnil;
}

static VALUE request_remote_host(VALUE self)
{
    request_data *data;
    const char *host;

    Data_Get_Struct(self, request_data, data);
    host = ap_get_remote_host(data->request->connection,
			      data->request->per_dir_config,
			      REMOTE_HOST);
    return host ? rb_str_new2(host) : Qnil;
}

static VALUE request_remote_logname(VALUE self)
{
    request_data *data;
    const char *logname;

    Data_Get_Struct(self, request_data, data);
    logname = ap_get_remote_logname(data->request);
    return logname ? rb_str_new2(logname) : Qnil;
}

static VALUE request_server_name(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    return rb_str_new2(ap_get_server_name(data->request));
}

static VALUE request_server_port(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    return INT2NUM(ap_get_server_port(data->request));
}

static VALUE request_escape_html(VALUE self, VALUE str)
{
    request_data *data;
    char *tmp;

    Data_Get_Struct(self, request_data, data);
    tmp = ap_escape_html(data->request->pool, STR2CSTR(str));
    return rb_str_new2(tmp);
}

static VALUE request_signature(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    return rb_str_new2(ap_psignature("", data->request));
}

static VALUE request_reset_timeout(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    ap_reset_timeout(data->request);
    return Qnil;
}

static VALUE request_hard_timeout(VALUE self, VALUE name)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    ap_hard_timeout(ap_pstrdup(data->request->pool, STR2CSTR(name)),
		    data->request);
    return Qnil;
}

static VALUE request_soft_timeout(VALUE self, VALUE name)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    ap_soft_timeout(ap_pstrdup(data->request->pool, STR2CSTR(name)),
		    data->request);
    return Qnil;
}

static VALUE request_kill_timeout(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    ap_kill_timeout(data->request);
    return Qnil;
}

static VALUE request_add_common_vars(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    ap_add_common_vars(data->request);
    return Qnil;
}

static VALUE request_add_cgi_vars(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    ap_add_cgi_vars(data->request);
    return Qnil;
}

void ruby_init_apachelib()
{
    rb_define_global_function("p", f_p, -1);
    rb_define_global_function("exit", f_exit, -1);

    rb_mApache = rb_define_module("Apache");
    rb_define_module_function(rb_mApache, "server_version", apache_server_version, 0);
    rb_define_module_function(rb_mApache, "add_version_component",
			      apache_add_version_component, 1);
    rb_define_module_function(rb_mApache, "server_built", apache_server_built, 0);
    rb_define_module_function(rb_mApache, "request", apache_request, 0);
    rb_define_module_function(rb_mApache, "unescape_url", apache_unescape_url, 1);
    rb_define_module_function(rb_mApache, "chdir_file", apache_chdir_file, 1);

    rb_cApacheTable = rb_define_class_under(rb_mApache, "Table", rb_cObject);
    rb_include_module(rb_cApacheTable, rb_mEnumerable);
    rb_undef_method(CLASS_OF(rb_cApacheTable), "new");
    rb_define_method(rb_cApacheTable, "clear", table_clear, 0);
    rb_define_method(rb_cApacheTable, "get", table_get, 1);
    rb_define_method(rb_cApacheTable, "[]", table_get, 1);
    rb_define_method(rb_cApacheTable, "set", table_set, 2);
    rb_define_method(rb_cApacheTable, "[]=", table_set, 2);
    rb_define_method(rb_cApacheTable, "setn", table_setn, 2);
    rb_define_method(rb_cApacheTable, "merge", table_merge, 2);
    rb_define_method(rb_cApacheTable, "mergen", table_mergen, 2);
    rb_define_method(rb_cApacheTable, "unset", table_unset, 1);
    rb_define_method(rb_cApacheTable, "add", table_add, 2);
    rb_define_method(rb_cApacheTable, "addn", table_addn, 2);
    rb_define_method(rb_cApacheTable, "each", table_each, 0);
    rb_define_method(rb_cApacheTable, "each_key", table_each_key, 0);
    rb_define_method(rb_cApacheTable, "each_value", table_each_value, 0);
    rb_cApacheInTable = rb_define_class_under(rb_mApache, "InTable",
					      rb_cApacheTable);
    rb_define_method(rb_cApacheInTable, "get", in_table_get, 1);
    rb_define_method(rb_cApacheInTable, "[]", in_table_get, 1);
    rb_define_method(rb_cApacheInTable, "each", in_table_each, 0);
    rb_define_method(rb_cApacheInTable, "each_key", in_table_each_key, 0);
    rb_define_method(rb_cApacheInTable, "each_value", in_table_each_value, 0);

    rb_cApacheConnection = rb_define_class_under(rb_mApache, "Connection", rb_cObject);
    rb_undef_method(CLASS_OF(rb_cApacheConnection), "new");
    rb_define_method(rb_cApacheConnection, "remote_ip",
		     connection_remote_ip, 0);
    rb_define_method(rb_cApacheConnection, "remote_host",
		     connection_remote_host, 0);
    rb_define_method(rb_cApacheConnection, "remote_logname",
		     connection_remote_logname, 0);
    rb_define_method(rb_cApacheConnection, "user", connection_user, 0);
    rb_define_method(rb_cApacheConnection, "auth_type",
		     connection_auth_type, 0);
    rb_define_method(rb_cApacheConnection, "local_ip",
		     connection_local_ip, 0);
    rb_define_method(rb_cApacheConnection, "local_host",
		     connection_local_host, 0);

    rb_cApacheServer = rb_define_class_under(rb_mApache, "Server", rb_cObject);
    rb_undef_method(CLASS_OF(rb_cApacheConnection), "new");
    rb_define_method(rb_cApacheServer, "defn_name", server_defn_name, 0);
    rb_define_method(rb_cApacheServer, "defn_line_number",
		     server_defn_line_number, 0);
    rb_define_method(rb_cApacheServer, "srm_confname", server_srm_confname, 0);
    rb_define_method(rb_cApacheServer, "access_confname", server_access_confname, 0);
    rb_define_method(rb_cApacheServer, "admin", server_admin, 0);
    rb_define_method(rb_cApacheServer, "hostname", server_hostname, 0);
    rb_define_method(rb_cApacheServer, "port", server_port, 0);
    rb_define_method(rb_cApacheServer, "error_fname", server_error_fname, 0);
    rb_define_method(rb_cApacheServer, "loglevel", server_loglevel, 0);
    rb_define_method(rb_cApacheServer, "is_virtual", server_is_virtual, 0);
    rb_define_method(rb_cApacheServer, "virtual?", server_is_virtual, 0);

    rb_cApacheRequest = rb_define_class_under(rb_mApache, "Request", rb_cObject);
    rb_include_module(rb_cApacheRequest, rb_mEnumerable);
    rb_undef_method(CLASS_OF(rb_cApacheRequest), "new");
    rb_define_method(rb_cApacheRequest, "to_s", request_to_s, 0);
    rb_define_method(rb_cApacheRequest, "replace", request_replace, -1);
    rb_define_method(rb_cApacheRequest, "cancel", request_cancel, 0);
    rb_define_method(rb_cApacheRequest, "write", request_write, 1);
    rb_define_method(rb_cApacheRequest, "putc", request_putc, 1);
    rb_define_method(rb_cApacheRequest, "print", request_print, -1);
    rb_define_method(rb_cApacheRequest, "printf", request_printf, -1);
    rb_define_method(rb_cApacheRequest, "puts", request_puts, -1);
    rb_define_method(rb_cApacheRequest, "<<", request_addstr, 1);
    rb_define_method(rb_cApacheRequest, "send_http_header",
		     rb_request_send_http_header, 0);
    rb_define_method(rb_cApacheRequest, "connection", request_connection, 0);
    rb_define_method(rb_cApacheRequest, "server", request_server, 0);
    rb_define_method(rb_cApacheRequest, "protocol", request_protocol, 0);
    rb_define_method(rb_cApacheRequest, "hostname", request_hostname, 0);
    rb_define_method(rb_cApacheRequest, "unparsed_uri",
		     request_unparsed_uri, 0);
    rb_define_method(rb_cApacheRequest, "uri", request_uri, 0);
    rb_define_method(rb_cApacheRequest, "filename", request_get_filename, 0);
    rb_define_method(rb_cApacheRequest, "filename=", request_set_filename, 1);
    rb_define_method(rb_cApacheRequest, "path_info", request_path_info, 0);
    rb_define_method(rb_cApacheRequest, "request_time",
		     request_request_time, 0);
    rb_define_method(rb_cApacheRequest, "status", request_get_status, 0);
    rb_define_method(rb_cApacheRequest, "status=", request_set_status, 1);
    rb_define_method(rb_cApacheRequest, "status_line", request_get_status_line, 0);
    rb_define_method(rb_cApacheRequest, "status_line=",
		     request_set_status_line, 1);
    rb_define_method(rb_cApacheRequest, "request_method",
		     request_request_method, 0);
    rb_define_method(rb_cApacheRequest, "header_only?", request_header_only, 0);
    rb_define_method(rb_cApacheRequest, "args", request_args, 0);
    rb_define_method(rb_cApacheRequest, "content_length",
		     request_content_length, 0);
    rb_define_method(rb_cApacheRequest, "content_type",
		     request_get_content_type, 0);
    rb_define_method(rb_cApacheRequest, "content_type=",
		     request_set_content_type, 1);
    rb_define_method(rb_cApacheRequest, "content_encoding",
		     request_get_content_encoding, 0);
    rb_define_method(rb_cApacheRequest, "content_encoding=",
		     request_set_content_encoding, 1);
    rb_define_method(rb_cApacheRequest, "content_languages",
		     request_get_content_languages, 0);
    rb_define_method(rb_cApacheRequest, "content_languages=",
		     request_set_content_languages, 1);
    rb_define_method(rb_cApacheRequest, "headers_in", request_headers_in, 0);
    rb_define_method(rb_cApacheRequest, "headers_out", request_headers_out, 0);
    rb_define_method(rb_cApacheRequest, "err_headers_out",
		     request_err_headers_out, 0);
    rb_define_method(rb_cApacheRequest, "subprocess_env",
		     request_subprocess_env, 0);
    rb_define_method(rb_cApacheRequest, "notes", request_notes, 0);
    rb_define_method(rb_cApacheRequest, "finfo", request_finfo, 0);
    rb_define_method(rb_cApacheRequest, "[]", request_aref, 1);
    rb_define_method(rb_cApacheRequest, "[]=", request_aset, 2);
    rb_define_method(rb_cApacheRequest, "each_header", request_each_header, 0);
    rb_define_method(rb_cApacheRequest, "each_key", request_each_key, 0);
    rb_define_method(rb_cApacheRequest, "each_value", request_each_value, 0);
    rb_define_method(rb_cApacheRequest, "setup_client_block",
		     request_setup_client_block, -1);
    rb_define_method(rb_cApacheRequest, "should_client_block",
		     request_should_client_block, 0);
    rb_define_method(rb_cApacheRequest, "should_client_block?",
		     request_should_client_block, 0);
    rb_define_method(rb_cApacheRequest, "get_client_block",
		     request_get_client_block, 1);
    rb_define_method(rb_cApacheRequest, "read", request_read, -1);
    rb_define_method(rb_cApacheRequest, "getc", request_getc, 0);
    rb_define_method(rb_cApacheRequest, "eof", request_eof, 0);
    rb_define_method(rb_cApacheRequest, "eof?", request_eof, 0);
    rb_define_method(rb_cApacheRequest, "binmode", request_binmode, 0);
    rb_define_method(rb_cApacheRequest, "remote_host", request_remote_host, 0);
    rb_define_method(rb_cApacheRequest, "remote_logname",
		     request_remote_logname, 0);
    rb_define_method(rb_cApacheRequest, "server_name", request_server_name, 0);
    rb_define_method(rb_cApacheRequest, "server_port", request_server_port, 0);
    rb_define_method(rb_cApacheRequest, "escape_html", request_escape_html, 1);
    rb_define_method(rb_cApacheRequest, "signature", request_signature, 0);
    rb_define_method(rb_cApacheRequest, "reset_timeout",
		     request_reset_timeout, 0);
    rb_define_method(rb_cApacheRequest, "hard_timeout",
		     request_hard_timeout, 1);
    rb_define_method(rb_cApacheRequest, "soft_timeout",
		     request_soft_timeout, 1);
    rb_define_method(rb_cApacheRequest, "kill_timeout",
		     request_kill_timeout, 0);
    rb_define_method(rb_cApacheRequest, "add_common_vars",
		     request_add_common_vars, 0);
    rb_define_method(rb_cApacheRequest, "add_cgi_vars",
		     request_add_cgi_vars, 0);

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
}

/*
 * Local variables:
 * mode: C
 * tab-width: 8
 * End:
 */
