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

#ifndef APACHELIB_H
#define APACHELIB_H

/* apachelib.c */
extern VALUE rb_mApache;
extern VALUE rb_eApacheTimeoutError;
extern VALUE rb_request;
void rb_init_apache();
void rb_apache_exit(int status);

/* array_header.c */
extern VALUE rb_cApacheArrayHeader;
void rb_init_apache_array();
VALUE rb_apache_array_new(array_header *arr);

/* table.c */
extern VALUE rb_cApacheTable;
extern VALUE rb_cApacheInTable;
void rb_init_apache_table();
VALUE rb_apache_table_new(VALUE klass, table *tbl);

/* connection.c */
extern VALUE rb_cApacheConnection;
void rb_init_apache_connection();
VALUE rb_apache_connection_new(conn_rec *conn);

/* server.c */
extern VALUE rb_cApacheServer;
void rb_init_apache_server();
VALUE rb_apache_server_new(server_rec *server);

/* request.c */
extern VALUE rb_cApacheRequest;
void rb_init_apache_request();
VALUE rb_apache_request_new(request_rec *r);
long rb_apache_request_length(VALUE self);
VALUE rb_apache_request_send_http_header(VALUE self);
void rb_apache_request_flush(VALUE request);

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

#endif /* !APACHELIB_H */

/*
 * Local variables:
 * mode: C
 * tab-width: 8
 * End:
 */
