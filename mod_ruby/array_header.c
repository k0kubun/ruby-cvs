/*
 * $Id$
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

VALUE rb_cApacheArrayHeader;

VALUE rb_apache_array_new(array_header *arr)
{
    return Data_Wrap_Struct(rb_cApacheArrayHeader, NULL, NULL, arr);
}

static VALUE array_aref(VALUE self, VALUE idx)
{
    array_header *arr;
    int n;

    Data_Get_Struct(self, array_header, arr);
    n = NUM2INT(idx);
    if (n < 0) {
	n += arr->nelts;
	if (n < 0) {
	    rb_raise(rb_eIndexError, "index %d out of array", n - arr->nelts);
	}
    }
    else if (n >= arr->nelts) {
	rb_raise(rb_eIndexError, "index %d out of array", n);
    }
    return rb_str_new2(((char **) arr->elts)[n]);
}

static VALUE array_aset(VALUE self, VALUE idx, VALUE val)
{
    array_header *arr;
    int n;

    Data_Get_Struct(self, array_header, arr);
    n = NUM2INT(idx);
    if (n < 0) {
	n += arr->nelts;
	if (n < 0) {
	    rb_raise(rb_eIndexError, "index %d out of array", n - arr->nelts);
	}
    }
    else if (n >= arr->nelts) {
	rb_raise(rb_eIndexError, "index %d out of array", n);
    }
    ((char **) arr->elts)[n] = ap_pstrdup(arr->pool, STR2CSTR(val));
    return val;
}

static VALUE array_each(VALUE self)
{
    array_header *arr;
    int i;

    Data_Get_Struct(self, array_header, arr);
    for (i = 0; i < arr->nelts; i++) {
	rb_yield(rb_str_new2(((char **) arr->elts)[i]));
    }
    return Qnil;
}

void rb_init_apache_array()
{
    rb_cApacheArrayHeader = rb_define_class_under(rb_mApache, "ArrayHeader",
						  rb_cObject);
    rb_include_module(rb_cApacheArrayHeader, rb_mEnumerable);
    rb_undef_method(CLASS_OF(rb_cApacheArrayHeader), "new");
    rb_define_method(rb_cApacheArrayHeader, "[]", array_aref, 1);
    rb_define_method(rb_cApacheArrayHeader, "[]=", array_aset, 2);
    rb_define_method(rb_cApacheArrayHeader, "each", array_each, 0);
}
