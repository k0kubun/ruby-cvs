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
}
