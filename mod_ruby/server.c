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

VALUE rb_cApacheServer;

VALUE rb_apache_server_new(server_rec *server)
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
DEFINE_INT_ATTR_READER(server_timeout, server_rec, timeout);
DEFINE_INT_ATTR_READER(server_keep_alive_timeout, server_rec,
		       keep_alive_timeout);
DEFINE_INT_ATTR_READER(server_keep_alive_max, server_rec, keep_alive_max);
DEFINE_BOOL_ATTR_READER(server_keep_alive, server_rec, keep_alive);
DEFINE_INT_ATTR_READER(server_send_buffer_size, server_rec, send_buffer_size);
DEFINE_STRING_ATTR_READER(server_path, server_rec, path);
DEFINE_INT_ATTR_READER(server_uid, server_rec, server_uid);
DEFINE_INT_ATTR_READER(server_gid, server_rec, server_gid);
DEFINE_INT_ATTR_READER(server_limit_req_line, server_rec, limit_req_line);
DEFINE_INT_ATTR_READER(server_limit_req_fieldsize, server_rec,
		       limit_req_fieldsize);
DEFINE_INT_ATTR_READER(server_limit_req_fields, server_rec, limit_req_fields);

static VALUE server_names(VALUE self)
{
    server_rec *server;

    Data_Get_Struct(self, server_rec, server);
    if (server->names) {
	return rb_apache_array_new(server->names);
    }
    else {
	return Qnil;
    }
}

static VALUE server_wild_names(VALUE self)
{
    server_rec *server;

    Data_Get_Struct(self, server_rec, server);
    if (server->wild_names) {
	return rb_apache_array_new(server->wild_names);
    }
    else {
	return Qnil;
    }
}

void rb_init_apache_server()
{
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
    rb_define_method(rb_cApacheServer, "timeout", server_timeout, 0);
    rb_define_method(rb_cApacheServer, "keep_alive_timeout",
		     server_keep_alive_timeout, 0);
    rb_define_method(rb_cApacheServer, "keep_alive_max",
		     server_keep_alive_max, 0);
    rb_define_method(rb_cApacheServer, "keep_alive", server_keep_alive, 0);
    rb_define_method(rb_cApacheServer, "keep_alive?", server_keep_alive, 0);
    rb_define_method(rb_cApacheServer, "send_buffer_size",
		     server_send_buffer_size, 0);
    rb_define_method(rb_cApacheServer, "path", server_path, 0);
    rb_define_method(rb_cApacheServer, "names", server_names, 0);
    rb_define_method(rb_cApacheServer, "wild_names", server_wild_names, 0);
    rb_define_method(rb_cApacheServer, "uid", server_uid, 0);
    rb_define_method(rb_cApacheServer, "gid", server_gid, 0);
    rb_define_method(rb_cApacheServer, "limit_req_line",
		     server_limit_req_line, 0);
    rb_define_method(rb_cApacheServer, "limit_req_fieldsize",
		     server_limit_req_fieldsize, 0);
    rb_define_method(rb_cApacheServer, "limit_req_fields",
		     server_limit_req_fields, 0);
}
