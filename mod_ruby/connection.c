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

VALUE rb_cApacheConnection;

VALUE rb_apache_connection_new(conn_rec *conn)
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

void rb_init_apache_connection()
{
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
}
