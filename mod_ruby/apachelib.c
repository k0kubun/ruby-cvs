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
VALUE rb_cApacheRequest;
VALUE rb_eApacheTimeoutError;

#define STRING_LITERAL(s) rb_str_new(s, sizeof(s) - 1)

static VALUE apache_server_version(VALUE self)
{
    return rb_str_new2(ap_get_server_version());
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
    char *buff;

    Check_Type(url, T_STRING);
    buff = ALLOCA_N(char, RSTRING(url)->len + 1);
    memcpy(buff, RSTRING(url)->ptr, RSTRING(url)->len);
    buff[RSTRING(url)->len] = '\0';
    ap_unescape_url(buff);
    return rb_str_new2(buff);
}

static void free_table(table *tbl)
{
    /* do nothing */
}

static VALUE ruby_create_table(VALUE klass, table *tbl)
{
    return Data_Wrap_Struct(klass, 0, free_table, tbl);
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
    if (res == NULL)
	return Qnil;
    else
	return rb_str_new2(res);
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
    if (res == NULL)
	return Qnil;
    else
	return rb_str_new2(res);
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

static void request_mark(request_data *data)
{
    rb_gc_mark(data->inbuf);
    rb_gc_mark(data->outbuf);
    rb_gc_mark(data->headers_in);
    rb_gc_mark(data->headers_out);
}

VALUE ruby_create_request(request_rec *r)
{
    request_data *data;
    VALUE result;
    char buff[BUFSIZ];
    int len;
    
    r->content_type = "text/html";
    result = Data_Make_Struct(rb_cApacheRequest, request_data,
			      (void (*) _((void*))) request_mark, free, data);
    data->request = r;
    data->inbuf = rb_tainted_str_new("", 0);
    data->outbuf = rb_tainted_str_new("", 0);
    data->headers_in = ruby_create_table(rb_cApacheInTable, r->headers_in);
    data->headers_out = ruby_create_table(rb_cApacheTable, r->headers_out);
    data->send_http_header = 0;
    data->pos = 0;

    ap_hard_timeout("get_client_block", r);
    if (ap_should_client_block(r)) {
	while ((len = ap_get_client_block(r, buff, BUFSIZ)) > 0) {
	    rb_str_cat(data->inbuf, buff, len);
	}
    }
    ap_kill_timeout(r);

    return result;
}

int ruby_request_outbuf_length(VALUE self)
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

static VALUE request_hostname(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    return rb_str_new2(data->request->hostname);
}

static VALUE request_unparsed_uri(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    return rb_str_new2(data->request->unparsed_uri);
}

static VALUE request_uri(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    return rb_str_new2(data->request->uri);
}

static VALUE request_filename(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    return rb_str_new2(data->request->filename);
}

static VALUE request_path_info(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    if (data->request->path_info)
	return rb_str_new2(data->request->path_info);
    else
	return Qnil;
}

static VALUE request_request_time(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    return rb_time_new(data->request->request_time, 0);
}

static VALUE request_status_line(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    if (data->request->status_line)
	return rb_str_new2(data->request->status_line);
    else
	return Qnil;
}

static VALUE request_set_status_line(VALUE self, VALUE str)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    data->request->status_line =
	ap_pstrdup(data->request->pool, STR2CSTR(str));
    return str;
}

static VALUE request_request_method(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    return rb_str_new2(data->request->method);
}

static VALUE request_header_only(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    return data->request->header_only ? Qtrue : Qfalse;
}

static VALUE request_args(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    if (data->request->args)
	return rb_str_new2(data->request->args);
    else
	return Qnil;
}

static VALUE request_content_length(VALUE self)
{
    request_data *data;
    const char *s;

    Data_Get_Struct(self, request_data, data);
    s = ap_table_get(data->request->headers_in, "Content-Length");
#if defined(RUBY_VERSION_CODE) && RUBY_VERSION_CODE >= 150
    return s ? rb_cstr2inum(s, 10) : Qnil;
#else
    return s ? rb_str2inum(s, 10) : Qnil;
#endif
}

static VALUE request_get_content_type(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    if (data->request->content_type)
	return rb_str_new2(data->request->content_type);
    else
	return Qnil;
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

static VALUE request_get_content_encoding(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    if (data->request->content_encoding)
	return rb_str_new2(data->request->content_encoding);
    else
	return Qnil;
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
    return data->headers_in;
}

static VALUE request_headers_out(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    return data->headers_out;
}

static VALUE request_aref(VALUE self, VALUE vkey)
{
    request_data *data;
    char *key = STR2CSTR(vkey);
    const char *val;

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

static VALUE request_escape_html(VALUE self, VALUE str)
{
    request_data *data;
    char *tmp;

    Data_Get_Struct(self, request_data, data);
    tmp = ap_escape_html(data->request->pool, STR2CSTR(str));
    return rb_str_new2(tmp);
}

static VALUE request_read_all(VALUE self)
{
    request_data *data;
    VALUE str;

    Data_Get_Struct(self, request_data, data);
    if (data->pos >= RSTRING(data->inbuf)->len)
	return Qnil;
    str = rb_str_substr(data->inbuf, data->pos,
			RSTRING(data->inbuf)->len - data->pos);
    data->pos = RSTRING(data->inbuf)->len;
    OBJ_TAINT(str);
    return str;
}

static VALUE request_read(int argc, VALUE *argv, VALUE self)
{
    request_data *data;
    VALUE length, str;
    long len, rest;

    Data_Get_Struct(self, request_data, data);
    rb_scan_args(argc, argv, "01", &length);
    if (NIL_P(length)) {
	return request_read_all(self);
    }

    len = NUM2LONG(length);
    if (len < 0) {
	rb_raise(rb_eArgError, "negative length %d given", len);
    }

    if (data->pos >= RSTRING(data->inbuf)->len)
	return Qnil;
    if (len == 0)
	return rb_str_new("", 0);

    rest = RSTRING(data->inbuf)->len - data->pos;
    if (len > rest) len = rest;

    str = rb_str_substr(data->inbuf, data->pos, len);
    data->pos += len;
    OBJ_TAINT(str);
    return str;
}

static long memsearch(char *s, size_t slen, char *t, size_t tlen)
{
    long i, j, k;

    if (tlen == 0)
	return -1;
    for (i = 0; i < slen; i++) {
	for (j = i, k = 0; s[j] == t[k]; j++, k++) {
	    if (k == tlen - 1)
		return i;
	}
    }
    return -1;
}

static VALUE request_gets(int argc, VALUE *argv, VALUE self)
{
    request_data *data;
    VALUE rs, str;
    char *rsptr;
    long rslen, len, i;

    Data_Get_Struct(self, request_data, data);
    if (rb_scan_args(argc, argv, "01", &rs) == 0) {
	rs = rb_rs;
    }
    else {
	if (!NIL_P(rs)) Check_Type(rs, T_STRING);
    }

    if (data->pos >= RSTRING(data->inbuf)->len)
	return Qnil;

    if (NIL_P(rs)) {
	str = request_read_all(self);
	if (!NIL_P(str)) {
	    rb_lastline_set(str);
	}
	return str;
    }
    else {
	rsptr = RSTRING(rs)->ptr;
	rslen = RSTRING(rs)->len;
    }
    i = memsearch(RSTRING(data->inbuf)->ptr + data->pos,
		  RSTRING(data->inbuf)->len - data->pos,
		  rsptr, rslen);
    if (i == -1) {
	len = RSTRING(data->inbuf)->len - data->pos;
    }
    else {
	len = i + rslen;
    }
    str = rb_str_substr(data->inbuf, data->pos, len);
    data->pos += len;

    OBJ_TAINT(str);
    rb_lastline_set(str);
    return str;
}

static VALUE request_readline(int argc, VALUE *argv, VALUE self)
{
    VALUE line = request_gets(argc, argv, self);

    if (NIL_P(line)) {
	rb_raise(rb_eEOFError, "End of file reached");
    }
    return line;
}

static VALUE request_readlines(int argc, VALUE *argv, VALUE self)
{
    VALUE line, ary;

    ary = rb_ary_new();
    while (!NIL_P(line = request_gets(argc, argv, self))) {
	rb_ary_push(ary, line);
    }
    return ary;
}

static VALUE request_each_line(int argc, VALUE *argv, VALUE self)
{
    VALUE line;

    while (!NIL_P(line = request_gets(argc, argv, self))) {
	rb_yield(line);
    }
    return self;
}

static VALUE request_each_byte(int argc, VALUE *argv, VALUE self)
{
    request_data *data;
    unsigned char c;

    Data_Get_Struct(self, request_data, data);
    while (data->pos < RSTRING(data->inbuf)->len) {
	c = RSTRING(data->inbuf)->ptr[data->pos];
	rb_yield(INT2FIX(c));
	data->pos++;
    }
    return self;
}

static VALUE request_getc(VALUE self)
{
    request_data *data;
    unsigned char c;

    Data_Get_Struct(self, request_data, data);
    if (data->pos >= RSTRING(data->inbuf)->len)
	return Qnil;
    c = RSTRING(data->inbuf)->ptr[data->pos];
    data->pos++;
    return INT2FIX(c);
}

static VALUE request_readchar(VALUE self)
{
    VALUE c = request_getc(self);

    if (NIL_P(c)) {
	rb_raise(rb_eEOFError, "End of file reached");
    }
    return c;
}

static VALUE request_ungetc(VALUE self, VALUE c)
{
    request_data *data;
    int cc = NUM2INT(c);

    Data_Get_Struct(self, request_data, data);
    if (data->pos == 0)
	return Qnil;
    data->pos--;
    RSTRING(data->inbuf)->ptr[data->pos] = cc;
    return Qnil;
}

static VALUE request_tell(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    return INT2NUM(data->pos);
}

static VALUE request_seek(int argc, VALUE *argv, VALUE self)
{
    request_data *data;
    VALUE offset, ptrname;
    int whence;

    Data_Get_Struct(self, request_data, data);
    rb_scan_args(argc, argv, "11", &offset, &ptrname);
    if (argc == 1) whence = SEEK_SET;
    else whence = NUM2INT(ptrname);

    switch (whence) {
    case SEEK_SET:
	data->pos = NUM2LONG(offset);
	break;
    case SEEK_CUR:
	data->pos = data->pos + NUM2LONG(offset);
	break;
    case SEEK_END:
	data->pos = RSTRING(data->inbuf)->len + NUM2LONG(offset);
	break;
    default:
	rb_raise(rb_eArgError, "invalid whence: %d", whence);
	break;
    }
    if (data->pos < 0)
	data->pos = 0;
    if (data->pos > RSTRING(data->inbuf)->len)
	data->pos = RSTRING(data->inbuf)->len;

    return INT2FIX(0);
}

static VALUE request_rewind(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    data->pos = 0;
    return INT2FIX(0);
}

static VALUE request_set_pos(VALUE self, VALUE pos)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    data->pos = NUM2LONG(pos);
    if (data->pos < 0)
	data->pos = 0;
    if (data->pos > RSTRING(data->inbuf)->len)
	data->pos = RSTRING(data->inbuf)->len;
    return INT2NUM(pos);
}

static VALUE request_eof(VALUE self)
{
    request_data *data;

    Data_Get_Struct(self, request_data, data);
    return data->pos >= RSTRING(data->inbuf)->len ? Qtrue : Qfalse;
}

static VALUE request_binmode(VALUE self)
{
    return Qnil;
}

void ruby_init_apachelib()
{
    rb_mApache = rb_define_module("Apache");
    rb_define_module_function(rb_mApache, "server_version", apache_server_version, 0);
    rb_define_module_function(rb_mApache, "server_built", apache_server_built, 0);
    rb_define_module_function(rb_mApache, "request", apache_request, 0);
    rb_define_module_function(rb_mApache, "unescape_url", apache_unescape_url, 1);

    rb_cApacheTable = rb_define_class_under(rb_mApache, "Table", rb_cObject);
    rb_include_module(rb_cApacheTable, rb_mEnumerable);
    rb_undef_method(CLASS_OF(rb_cApacheTable), "new");
    rb_define_method(rb_cApacheTable, "clear", table_clear, 0);
    rb_define_method(rb_cApacheTable, "get", table_get, 1);
    rb_define_alias(rb_cApacheTable, "[]", "get");
    rb_define_method(rb_cApacheTable, "set", table_set, 2);
    rb_define_alias(rb_cApacheTable, "[]=", "set");
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
    rb_define_method(rb_cApacheInTable, "each", in_table_each, 0);
    rb_define_method(rb_cApacheInTable, "each_key", in_table_each_key, 0);
    rb_define_method(rb_cApacheInTable, "each_value", in_table_each_value, 0);

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
    rb_define_method(rb_cApacheRequest, "hostname", request_hostname, 0);
    rb_define_method(rb_cApacheRequest, "unparsed_uri", request_unparsed_uri, 0);
    rb_define_method(rb_cApacheRequest, "uri", request_uri, 0);
    rb_define_method(rb_cApacheRequest, "filename", request_filename, 0);
    rb_define_method(rb_cApacheRequest, "path_info", request_path_info, 0);
    rb_define_method(rb_cApacheRequest, "request_time", request_request_time, 0);
    rb_define_method(rb_cApacheRequest, "status_line", request_status_line, 0);
    rb_define_method(rb_cApacheRequest, "status_line=", request_set_status_line, 1);
    rb_define_method(rb_cApacheRequest, "request_method", request_request_method, 0);
    rb_define_method(rb_cApacheRequest, "header_only?", request_header_only, 0);
    rb_define_method(rb_cApacheRequest, "args", request_args, 0);
    rb_define_method(rb_cApacheRequest, "content_length", request_content_length, 0);
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
    rb_define_method(rb_cApacheRequest, "[]", request_aref, 1);
    rb_define_method(rb_cApacheRequest, "[]=", request_aset, 2);
    rb_define_method(rb_cApacheRequest, "each_header", request_each_header, 0);
    rb_define_method(rb_cApacheRequest, "each_key", request_each_key, 0);
    rb_define_method(rb_cApacheRequest, "each_value", request_each_value, 0);
    rb_define_method(rb_cApacheRequest, "escape_html", request_escape_html, 1);
    rb_define_method(rb_cApacheRequest, "read", request_read, -1);
    rb_define_method(rb_cApacheRequest, "gets", request_gets, -1);
    rb_define_method(rb_cApacheRequest, "readline", request_readline, -1);
    rb_define_method(rb_cApacheRequest, "readlines", request_readlines, -1);
    rb_define_method(rb_cApacheRequest, "each", request_each_line, -1);
    rb_define_method(rb_cApacheRequest, "each_line", request_each_line, -1);
    rb_define_method(rb_cApacheRequest, "each_byte", request_each_byte, 0);
    rb_define_method(rb_cApacheRequest, "getc", request_getc, 0);
    rb_define_method(rb_cApacheRequest, "readchar", request_readchar, 0);
    rb_define_method(rb_cApacheRequest, "ungetc", request_ungetc, 1);
    rb_define_method(rb_cApacheRequest, "tell", request_tell, 0);
    rb_define_method(rb_cApacheRequest, "seek", request_seek, -1);
    rb_define_method(rb_cApacheRequest, "rewind", request_rewind, 0);
    rb_define_method(rb_cApacheRequest, "pos", request_tell, 0);
    rb_define_method(rb_cApacheRequest, "pos=", request_set_pos, 1);
    rb_define_method(rb_cApacheRequest, "eof", request_eof, 0);
    rb_define_method(rb_cApacheRequest, "eof?", request_eof, 0);
    rb_define_method(rb_cApacheRequest, "binmode", request_binmode, 0);

    rb_eApacheTimeoutError =
	rb_define_class_under(rb_mApache, "TimeoutError", rb_eException);

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
}

/*
 * Local variables:
 * mode: C
 * tab-width: 8
 * End:
 */
