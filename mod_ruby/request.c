/*
 * $Id$
 * Copyright (C) 2000  ZetaBITS, Inc.
 * Copyright (C) 2000  Information-technology Promotion Agency, Japan
 * Copyright (C) 2000  Shugo Maeda <shugo@modruby.net>
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

#include "mod_ruby.h"
#include "apachelib.h"

static VALUE rb_eApachePrematureChunkEndError;
VALUE rb_cApacheRequest;

typedef struct request_data {
    request_rec *request;
    VALUE outbuf;
    VALUE connection;
    VALUE server;
    VALUE headers_in;
    VALUE headers_out;
    VALUE err_headers_out;
    VALUE subprocess_env;
    VALUE finfo;
    VALUE error_message;
    VALUE exception;
} request_data;

#define REQ_SYNC_HEADER     FL_USER1
#define REQ_SYNC_OUTPUT     FL_USER2
#define REQ_HEADER_PENDING  FL_USER3
#define REQ_SENT_HEADER     FL_USER4

#define REQUEST_STRING_ATTR_READER(fname, member) \
	DEFINE_STRING_ATTR_READER(fname, request_data, request->member)

#define REQUEST_STRING_ATTR_WRITER(fname, member) \
static VALUE fname(VALUE self, VALUE val) \
{ \
    request_data *data; \
    data = get_request_data(self); \
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
    rb_gc_mark(data->finfo);
    rb_gc_mark(data->error_message);
    rb_gc_mark(data->exception);
}

static void cleanup_request_object(void *data)
{
    request_rec *r = (request_rec *) data;
    VALUE reqobj;

    reqobj = (VALUE) ap_get_module_config(r->request_config, &ruby_module);
    if (reqobj == 0) return;
    if (TYPE(reqobj) == T_DATA)
	RDATA(reqobj)->data = NULL;
    ap_set_module_config(r->request_config, &ruby_module, 0);
    rb_apache_unregister_object(reqobj);
}

static VALUE apache_request_new(request_rec *r)
{
    request_data *data;
    VALUE obj;

    obj = Data_Make_Struct(rb_cApacheRequest, request_data,
			   request_mark, free, data);
    data->request = r;
    data->outbuf = rb_tainted_str_new("", 0);
    data->connection = Qnil;
    data->server = Qnil;
    data->headers_in = Qnil;
    data->headers_out = Qnil;
    data->err_headers_out = Qnil;
    data->subprocess_env = Qnil;
    data->finfo = Qnil;
    data->error_message = Qnil;
    data->exception = Qnil;
    rb_apache_register_object(obj);
    ap_set_module_config(r->request_config, &ruby_module, (void *) obj);
    ap_register_cleanup(r->pool, (void *) r,
			cleanup_request_object, ap_null_cleanup);
    return obj;
}

VALUE rb_get_request_object(request_rec *r)
{
    VALUE reqobj;

    if (r == NULL) return Qnil;
    reqobj = (VALUE) ap_get_module_config(r->request_config, &ruby_module);
    if (reqobj) {
	return reqobj;
    }
    else {
	return apache_request_new(r);
    }
}

static request_data *get_request_data(VALUE obj)
{
    request_data *data;

    Check_Type(obj, T_DATA);
    data = (request_data *) RDATA(obj)->data;
    if (data == NULL)
	rb_raise(rb_eArgError, "destroyed request");
    return data;
}

long rb_apache_request_length(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    return RSTRING(data->outbuf)->len;
}

static VALUE request_output_buffer(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    return data->outbuf;
}

static VALUE request_replace(int argc, VALUE *argv, VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    return rb_funcall2(data->outbuf, rb_intern("replace"), argc, argv);
}

static VALUE request_cancel(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    RSTRING(data->outbuf)->len = 0;
    RSTRING(data->outbuf)->ptr[0] = '\0';
    return Qnil;
}

static VALUE request_get_sync_header(VALUE self)
{
    return FL_TEST(self, REQ_SYNC_HEADER) ? Qtrue : Qfalse;
}

static VALUE request_set_sync_header(VALUE self, VALUE val)
{
    if (RTEST(val)) {
	FL_SET(self, REQ_SYNC_HEADER);
    }
    else {
	FL_UNSET(self, REQ_SYNC_HEADER);
    }
    return val;
}

static VALUE request_get_sync_output(VALUE self)
{
    return FL_TEST(self, REQ_SYNC_OUTPUT) ? Qtrue : Qfalse;
}

static VALUE request_set_sync_output(VALUE self, VALUE val)
{
    if (RTEST(val)) {
	FL_SET(self, REQ_SYNC_OUTPUT);
    }
    else {
	FL_UNSET(self, REQ_SYNC_OUTPUT);
    }
    return val;
}

static VALUE request_set_sync(VALUE self, VALUE val)
{
    if (RTEST(val)) {
	FL_SET(self, REQ_SYNC_HEADER);
	FL_SET(self, REQ_SYNC_OUTPUT);
    }
    else {
	FL_UNSET(self, REQ_SYNC_HEADER);
	FL_UNSET(self, REQ_SYNC_OUTPUT);
    }
    return val;
}

static VALUE request_write(VALUE self, VALUE str)
{
    request_data *data;
    int len;

    data = get_request_data(self);
    str = rb_obj_as_string(str);
    if (FL_TEST(self, REQ_SYNC_OUTPUT)) {
	if (data->request->header_only &&
	    FL_TEST(self, REQ_SENT_HEADER))
	    return INT2NUM(0);
	len = ap_rwrite(RSTRING(str)->ptr, RSTRING(str)->len, data->request);
	ap_rflush(data->request);
    }
    else {
	rb_str_cat(data->outbuf, RSTRING(str)->ptr, RSTRING(str)->len);
	len = RSTRING(str)->len;
    }
    return INT2NUM(len);
}

static VALUE request_putc(VALUE self, VALUE c)
{
    char ch = NUM2CHR(c);
    request_data *data;

    data = get_request_data(self);
    if (FL_TEST(self, REQ_SYNC_OUTPUT)) {
	int ret;

	if (data->request->header_only &&
	    FL_TEST(self, REQ_SENT_HEADER))
	    return INT2NUM(EOF);
	ret = ap_rputc(NUM2INT(c), data->request);
	return INT2NUM(ret);
    }
    else {
	rb_str_cat(data->outbuf, &ch, 1);
	return c;
    }
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

VALUE rb_apache_request_send_http_header(VALUE self)
{
    request_data *data;

    if (FL_TEST(self, REQ_SYNC_HEADER)) {
	data = get_request_data(self);
	ap_send_http_header(data->request);
	FL_SET(self, REQ_SENT_HEADER);
	FL_UNSET(self, REQ_HEADER_PENDING);
    }
    else {
	FL_SET(self, REQ_HEADER_PENDING);
    }
    return Qnil;
}

static VALUE request_sent_http_header(VALUE self)
{
    if (FL_TEST(self, REQ_SENT_HEADER) ||
	FL_TEST(self, REQ_HEADER_PENDING)) {
	return Qtrue;
    }
    else {
	return Qfalse;
    }
}

void rb_apache_request_flush(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    if (FL_TEST(self, REQ_HEADER_PENDING)) {
	ap_send_http_header(data->request);
	FL_SET(self, REQ_SENT_HEADER);
	FL_UNSET(self, REQ_HEADER_PENDING);
    }
    if (data->request->header_only &&
	FL_TEST(self, REQ_SENT_HEADER)) {
	RSTRING(data->outbuf)->len = 0;
	return;
    }
    if (RSTRING(data->outbuf)->len > 0) {
	ap_rwrite(RSTRING(data->outbuf)->ptr, RSTRING(data->outbuf)->len,
		  data->request);
	ap_rflush(data->request);
	RSTRING(data->outbuf)->len = 0;
    }
}


static VALUE request_connection(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    if (NIL_P(data->connection)) {
	data->connection = rb_apache_connection_new(data->request->connection);
    }
    return data->connection;
}

static VALUE request_server(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    if (NIL_P(data->server)) {
	data->server = rb_apache_server_new(data->request->server);
    }
    return data->server;
}

static VALUE request_next(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    return rb_get_request_object(data->request->next);
}

static VALUE request_prev(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    return rb_get_request_object(data->request->prev);
}

static VALUE request_main(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    return rb_get_request_object(data->request->main);
}

REQUEST_STRING_ATTR_READER(request_protocol, protocol);
REQUEST_STRING_ATTR_READER(request_hostname, hostname);
REQUEST_STRING_ATTR_READER(request_unparsed_uri, unparsed_uri);
REQUEST_STRING_ATTR_READER(request_get_uri, uri);
REQUEST_STRING_ATTR_WRITER(request_set_uri, uri);
REQUEST_STRING_ATTR_READER(request_get_filename, filename);
REQUEST_STRING_ATTR_WRITER(request_set_filename, filename);
REQUEST_STRING_ATTR_READER(request_get_path_info, path_info);
REQUEST_STRING_ATTR_WRITER(request_set_path_info, path_info);
REQUEST_INT_ATTR_READER(request_get_status, status);
REQUEST_INT_ATTR_WRITER(request_set_status, status);
REQUEST_STRING_ATTR_READER(request_get_status_line, status_line);
REQUEST_STRING_ATTR_WRITER(request_set_status_line, status_line);
REQUEST_STRING_ATTR_READER(request_the_request, the_request);
REQUEST_STRING_ATTR_READER(request_request_method, method);
REQUEST_INT_ATTR_READER(request_method_number, method_number);
REQUEST_STRING_ATTR_READER(request_args, args);
REQUEST_STRING_ATTR_READER(request_get_content_type, content_type);
REQUEST_STRING_ATTR_READER(request_get_content_encoding, content_encoding);

static VALUE request_request_time(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    return rb_time_new(data->request->request_time, 0);
}

static VALUE request_header_only(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    return data->request->header_only ? Qtrue : Qfalse;
}

static VALUE request_content_length(VALUE self)
{
    request_data *data;
    const char *s;

    rb_warn("content_length is obsolete; use headers_in[\"Content-Length\"] instead");
    data = get_request_data(self);
    s = ap_table_get(data->request->headers_in, "Content-Length");
    return s ? rb_cstr2inum(s, 10) : Qnil;
}

static VALUE request_set_content_type(VALUE self, VALUE str)
{
    request_data *data;

    data = get_request_data(self);
    if (NIL_P(str)) {
	data->request->content_type = NULL;
    }
    else {
	Check_Type(str, T_STRING);
	str = rb_funcall(str, rb_intern("downcase"), 0);
	data->request->content_type =
	    ap_pstrdup(data->request->pool, RSTRING(str)->ptr);
    }
    return str;
}

static VALUE request_set_content_encoding(VALUE self, VALUE str)
{
    request_data *data;

    data = get_request_data(self);
    if (NIL_P(str)) {
	data->request->content_encoding = NULL;
    }
    else {
	Check_Type(str, T_STRING);
	str = rb_funcall(str, rb_intern("downcase"), 0);
	data->request->content_encoding =
	    ap_pstrdup(data->request->pool, RSTRING(str)->ptr);
    }
    return str;
}
 
static VALUE request_get_content_languages(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    if (data->request->content_languages) {
#if 0
	VALUE ary = rb_ary_new();
	int i, len =  data->request->content_languages->nelts;
	char **langs = (char **) data->request->content_languages->elts;
	for (i = 0; i < len; i++) {
	    rb_ary_push(ary, rb_str_new2(langs[i]));
	}
	rb_ary_freeze(ary);
	return ary;
#else
	return rb_apache_array_new(data->request->content_languages);
#endif	
    }
    else {
	return Qnil;
    }
}

static VALUE request_set_content_languages(VALUE self, VALUE ary)
{
    request_data *data;
    int i;

    data = get_request_data(self);
    if (NIL_P(ary)) {
	data->request->content_languages = NULL;
    }
    else {
	Check_Type(ary, T_ARRAY);
	for (i = 0; i < RARRAY(ary)->len; i++) {
	    Check_Type(RARRAY(ary)->ptr[i], T_STRING);
	}
	data->request->content_languages =
	    ap_make_array(data->request->pool, RARRAY(ary)->len, sizeof(char *));
	for (i = 0; i < RARRAY(ary)->len; i++) {
	    VALUE str = RARRAY(ary)->ptr[i];
	    str = rb_funcall(str, rb_intern("downcase"), 0);
	    *(char **) ap_push_array(data->request->content_languages) =
		ap_pstrdup(data->request->pool, STR2CSTR(str));
	}
    }
    return ary;
}

static VALUE request_headers_in(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    if (NIL_P(data->headers_in)) {
	if (ap_table_get(data->request->notes, "ruby_in_authen_handler")) {
	    data->headers_in = rb_apache_table_new(rb_cApacheTable,
						   data->request->headers_in);
	}
	else {
	    data->headers_in = rb_apache_table_new(rb_cApacheRestrictedTable,
						   data->request->headers_in);
	}
    }
    return data->headers_in;
}

static VALUE request_headers_out(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    if (NIL_P(data->headers_out)) {
	data->headers_out = rb_apache_table_new(rb_cApacheTable,
						data->request->headers_out);
    }
    return data->headers_out;
}

static VALUE request_err_headers_out(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    if (NIL_P(data->err_headers_out)) {
	data->err_headers_out =
	    rb_apache_table_new(rb_cApacheTable, data->request->err_headers_out);
    }
    return data->err_headers_out;
}

static VALUE request_subprocess_env(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    if (NIL_P(data->subprocess_env)) {
	data->subprocess_env =
	    rb_apache_table_new(rb_cApacheTable,
				data->request->subprocess_env);
    }
    return data->subprocess_env;
}

static VALUE request_finfo(VALUE self)
{
    VALUE cStat;
    request_data *data;
    struct stat *st;

    data = get_request_data(self);
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
    data = get_request_data(self);
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
	data = get_request_data(self);
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
    data = get_request_data(self);
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
    data = get_request_data(self);
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
    data = get_request_data(self);
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

    data = get_request_data(self);
    if (rb_scan_args(argc, argv, "01", &policy) == 1) {
	read_policy = NUM2INT(policy);
    }
    return INT2NUM(ap_setup_client_block(data->request, read_policy));
}

static VALUE request_should_client_block(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    return INT2BOOL(ap_should_client_block(data->request));
}

static VALUE request_get_client_block(VALUE self, VALUE length)
{
    request_data *data;
    char *buf;
    int len;

    data = get_request_data(self);
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
	    rb_apache_exit(rc);
        }
    }
    old_read_length = r->read_length;
    r->read_length = 0;
    if (ap_should_client_block(r)) {
	if (len < 0)
	    len = r->remaining;
	buf = (char *) ap_palloc(r->pool, len);
	result = rb_tainted_str_new("", 0);
	while (len > 0) {
	    nrd = ap_get_client_block(r, buf, len);
	    if (nrd == 0) {
		break;
	    }
	    if (nrd == -1) {
		r->read_length += old_read_length;
		rb_raise(rb_eApachePrematureChunkEndError, "premature chunk end");
	    }
	    rb_str_cat(result, buf, nrd);
	    len -= nrd;
	}
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

    data = get_request_data(self);
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

    data = get_request_data(self);
    str = read_client_block(data->request, 1);
    if (NIL_P(str) || RSTRING(str)->len == 0)
	return Qnil;
    return INT2FIX(RSTRING(str)->ptr[0]);
}

static VALUE request_eof(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    return INT2BOOL(data->request->remaining == 0);
}

static VALUE request_binmode(VALUE self)
{
    return Qnil;
}

static VALUE request_allow_options(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    return INT2NUM(ap_allow_options(data->request));
}

static VALUE request_allow_overrides(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    return INT2NUM(ap_allow_overrides(data->request));
}

static VALUE request_default_type(VALUE self)
{
    request_data *data;
    const char *type;

    data = get_request_data(self);
    type = ap_default_type(data->request);
    return type ? rb_str_new2(type) : Qnil;
}

static VALUE request_remote_host(VALUE self)
{
    request_data *data;
    const char *host;

    data = get_request_data(self);
    host = ap_get_remote_host(data->request->connection,
			      data->request->per_dir_config,
			      REMOTE_HOST);
    return host ? rb_str_new2(host) : Qnil;
}

static VALUE request_remote_logname(VALUE self)
{
    request_data *data;
    const char *logname;

    data = get_request_data(self);
    logname = ap_get_remote_logname(data->request);
    return logname ? rb_str_new2(logname) : Qnil;
}

static VALUE request_construct_url(VALUE self, VALUE uri)
{
    request_data *data;
    char *url;

    data = get_request_data(self);
    url = ap_construct_url(data->request->pool, STR2CSTR(uri), data->request);
    return rb_str_new2(url);
}

static VALUE request_server_name(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    return rb_str_new2(ap_get_server_name(data->request));
}

static VALUE request_server_port(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    return INT2NUM(ap_get_server_port(data->request));
}

static VALUE request_auth_type(VALUE self)
{
    request_data *data;
    const char *auth_type;

    data = get_request_data(self);
    auth_type = ap_auth_type(data->request);
    return auth_type ? rb_str_new2(auth_type) : Qnil;
}

static VALUE request_auth_name(VALUE self)
{
    request_data *data;
    const char *auth_name;

    data = get_request_data(self);
    auth_name = ap_auth_name(data->request);
    return auth_name ? rb_str_new2(auth_name) : Qnil;
}

static VALUE request_satisfies(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    return INT2NUM(ap_satisfies(data->request));
}

static VALUE request_requires(VALUE self)
{
    VALUE result, req;
    request_data *data;
    const array_header *reqs_arr;
    require_line *reqs;
    int i;

    data = get_request_data(self);
    reqs_arr = ap_requires(data->request);
    if (reqs_arr == NULL)
	return Qnil;
    reqs = (require_line *) reqs_arr->elts;
    result = rb_ary_new2(reqs_arr->nelts);
    for (i = 0; i < reqs_arr->nelts; i++) {
	req = rb_assoc_new(INT2NUM(reqs[i].method_mask),
			   rb_str_new2(reqs[i].requirement));
	rb_ary_push(result, req);
    }
    return result;
}

static VALUE request_escape_html(VALUE self, VALUE str)
{
    request_data *data;
    char *tmp;
    VALUE result;

    data = get_request_data(self);
    tmp = ap_escape_html(data->request->pool, STR2CSTR(str));
    result = rb_str_new2(tmp);
    OBJ_INFECT(result, str);
    return result;
}

static VALUE request_signature(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    return rb_str_new2(ap_psignature("", data->request));
}

static VALUE request_reset_timeout(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    ap_reset_timeout(data->request);
    return Qnil;
}

static VALUE request_hard_timeout(VALUE self, VALUE name)
{
    request_data *data;

    data = get_request_data(self);
    ap_hard_timeout(ap_pstrdup(data->request->pool, STR2CSTR(name)),
		    data->request);
    return Qnil;
}

static VALUE request_soft_timeout(VALUE self, VALUE name)
{
    request_data *data;

    data = get_request_data(self);
    ap_soft_timeout(ap_pstrdup(data->request->pool, STR2CSTR(name)),
		    data->request);
    return Qnil;
}

static VALUE request_kill_timeout(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    ap_kill_timeout(data->request);
    return Qnil;
}

static VALUE request_note_auth_failure(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    ap_note_auth_failure(data->request);
    return Qnil;
}

static VALUE request_note_basic_auth_failure(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    ap_note_basic_auth_failure(data->request);
    return Qnil;
}

static VALUE request_note_digest_auth_failure(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    ap_note_digest_auth_failure(data->request);
    return Qnil;
}

static VALUE request_get_basic_auth_pw(VALUE self)
{
    request_data *data;
    const char *pw;
    int res;

    data = get_request_data(self);
    if (ap_table_get(data->request->notes, "ruby_in_authen_handler") == NULL) {
	rb_raise(rb_eSecurityError, "Only RubyAuthenHandler can get password");
    }
    if ((res = ap_get_basic_auth_pw(data->request, &pw)) != OK) {
	rb_apache_exit(res);
    }
    return rb_str_new2(pw);
}

static VALUE request_add_common_vars(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    ap_add_common_vars(data->request);
    return Qnil;
}

static VALUE request_add_cgi_vars(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    ap_add_cgi_vars(data->request);
    return Qnil;
}

static VALUE request_setup_cgi_env(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    rb_setup_cgi_env(data->request);
    return Qnil;
}

static VALUE request_log_reason(VALUE self, VALUE msg, VALUE file)
{
    request_data *data;

    Check_Type(msg, T_STRING);
    Check_Type(file, T_STRING);
    data = get_request_data(self);
    ap_log_error(APLOG_MARK, APLOG_ERR | APLOG_NOERRNO,
		 data->request->server,
		 "access to %s failed for %s, reason: %s",
		 STR2CSTR(file),
		 ap_get_remote_host(data->request->connection,
				    data->request->per_dir_config,
				    REMOTE_NAME),
		 STR2CSTR(msg));
    return Qnil;
}

void rb_apache_request_set_error(VALUE request, VALUE errmsg, VALUE exception)
{
    request_data *data;

    Data_Get_Struct(request, request_data, data);
    data->error_message = errmsg;
    data->exception = exception;
}

static VALUE request_error_message(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    return data->error_message;
}

static VALUE request_exception(VALUE self)
{
    request_data *data;

    data = get_request_data(self);
    return data->exception;
}

void rb_init_apache_request()
{
    rb_eApachePrematureChunkEndError =
	rb_define_class_under(rb_mApache, "PrematureChunkEndError",
			      rb_eStandardError);

    rb_cApacheRequest = rb_define_class_under(rb_mApache, "Request", rb_cObject);
    rb_include_module(rb_cApacheRequest, rb_mEnumerable);
    rb_undef_method(CLASS_OF(rb_cApacheRequest), "new");
    rb_define_method(rb_cApacheRequest, "inspect", rb_any_to_s, 0);
    rb_define_method(rb_cApacheRequest, "to_s", request_output_buffer, 0);
    rb_define_method(rb_cApacheRequest, "output_buffer",
		     request_output_buffer, 0);
    rb_define_method(rb_cApacheRequest, "replace", request_replace, -1);
    rb_define_method(rb_cApacheRequest, "cancel", request_cancel, 0);
    rb_define_method(rb_cApacheRequest, "sync_header",
		     request_get_sync_header, 0);
    rb_define_method(rb_cApacheRequest, "sync_header=",
		     request_set_sync_header, 1);
    rb_define_method(rb_cApacheRequest, "sync_output",
		     request_get_sync_output, 0);
    rb_define_method(rb_cApacheRequest, "sync_output=",
		     request_set_sync_output, 1);
    rb_define_method(rb_cApacheRequest, "sync=",
		     request_set_sync, 1);
    rb_define_method(rb_cApacheRequest, "write", request_write, 1);
    rb_define_method(rb_cApacheRequest, "putc", request_putc, 1);
    rb_define_method(rb_cApacheRequest, "print", request_print, -1);
    rb_define_method(rb_cApacheRequest, "printf", request_printf, -1);
    rb_define_method(rb_cApacheRequest, "puts", request_puts, -1);
    rb_define_method(rb_cApacheRequest, "<<", request_addstr, 1);
    rb_define_method(rb_cApacheRequest, "send_http_header",
		     rb_apache_request_send_http_header, 0);
    rb_define_method(rb_cApacheRequest, "sent_http_header?",
		     request_sent_http_header, 0);
    rb_define_method(rb_cApacheRequest, "connection", request_connection, 0);
    rb_define_method(rb_cApacheRequest, "server", request_server, 0);
    rb_define_method(rb_cApacheRequest, "next", request_next, 0);
    rb_define_method(rb_cApacheRequest, "prev", request_prev, 0);
    rb_define_method(rb_cApacheRequest, "main", request_main, 0);
    rb_define_method(rb_cApacheRequest, "protocol", request_protocol, 0);
    rb_define_method(rb_cApacheRequest, "hostname", request_hostname, 0);
    rb_define_method(rb_cApacheRequest, "unparsed_uri",
		     request_unparsed_uri, 0);
    rb_define_method(rb_cApacheRequest, "uri", request_get_uri, 0);
    rb_define_method(rb_cApacheRequest, "uri=", request_set_uri, 1);
    rb_define_method(rb_cApacheRequest, "filename", request_get_filename, 0);
    rb_define_method(rb_cApacheRequest, "filename=", request_set_filename, 1);
    rb_define_method(rb_cApacheRequest, "path_info", request_get_path_info, 0);
    rb_define_method(rb_cApacheRequest, "path_info=", request_set_path_info, 1);
    rb_define_method(rb_cApacheRequest, "request_time",
		     request_request_time, 0);
    rb_define_method(rb_cApacheRequest, "status", request_get_status, 0);
    rb_define_method(rb_cApacheRequest, "status=", request_set_status, 1);
    rb_define_method(rb_cApacheRequest, "status_line", request_get_status_line, 0);
    rb_define_method(rb_cApacheRequest, "status_line=",
		     request_set_status_line, 1);
    rb_define_method(rb_cApacheRequest, "request_method",
		     request_request_method, 0);
    rb_define_method(rb_cApacheRequest, "method_number",
		     request_method_number, 0);
    rb_define_method(rb_cApacheRequest, "the_request",
		     request_the_request, 0);
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
    rb_define_method(rb_cApacheRequest, "allow_options",
		     request_allow_options, 0);
    rb_define_method(rb_cApacheRequest, "allow_overrides",
		     request_allow_overrides, 0);
    rb_define_method(rb_cApacheRequest, "default_type",
		     request_default_type, 0);
    rb_define_method(rb_cApacheRequest, "remote_host", request_remote_host, 0);
    rb_define_method(rb_cApacheRequest, "remote_logname",
		     request_remote_logname, 0);
    rb_define_method(rb_cApacheRequest, "construct_url",
		     request_construct_url, 1);
    rb_define_method(rb_cApacheRequest, "server_name", request_server_name, 0);
    rb_define_method(rb_cApacheRequest, "server_port", request_server_port, 0);
    rb_define_method(rb_cApacheRequest, "auth_type", request_auth_type, 0);
    rb_define_method(rb_cApacheRequest, "auth_name", request_auth_name, 0);
    rb_define_method(rb_cApacheRequest, "satisfies", request_satisfies, 0);
    rb_define_method(rb_cApacheRequest, "requires", request_requires, 0);
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
    rb_define_method(rb_cApacheRequest, "note_auth_failure",
		     request_note_auth_failure, 0);
    rb_define_method(rb_cApacheRequest, "note_basic_auth_failure",
		     request_note_basic_auth_failure, 0);
    rb_define_method(rb_cApacheRequest, "note_digest_auth_failure",
		     request_note_digest_auth_failure, 0);
    rb_define_method(rb_cApacheRequest, "get_basic_auth_pw",
		     request_get_basic_auth_pw, 0);
    rb_define_method(rb_cApacheRequest, "add_common_vars",
		     request_add_common_vars, 0);
    rb_define_method(rb_cApacheRequest, "add_cgi_vars",
		     request_add_cgi_vars, 0);
    rb_define_method(rb_cApacheRequest, "setup_cgi_env",
		     request_setup_cgi_env, 0);
    rb_define_method(rb_cApacheRequest, "log_reason",
		     request_log_reason, 2);
    rb_define_method(rb_cApacheRequest, "error_message",
		     request_error_message, 0);
    rb_define_method(rb_cApacheRequest, "exception",
		     request_exception, 0);
}
