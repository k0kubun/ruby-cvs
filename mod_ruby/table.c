/*
 * $Id$
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

VALUE rb_cApacheTable;
VALUE rb_cApacheInTable;

VALUE rb_apache_table_new(VALUE klass, table *tbl)
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

void rb_init_apache_table()
{
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
}
