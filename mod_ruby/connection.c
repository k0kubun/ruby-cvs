/*
 * $Id$
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
