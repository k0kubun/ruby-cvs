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

#ifndef APACHELIB_H
#define APACHELIB_H

extern VALUE rb_mApache;
extern VALUE rb_cApacheConnection;
extern VALUE rb_cApacheTable;
extern VALUE rb_cApacheInTable;
extern VALUE rb_cApacheRequest;
extern VALUE rb_eApacheTimeoutError;

VALUE ruby_create_request(request_rec *r, VALUE input);
long ruby_request_outbuf_length(VALUE self);
VALUE rb_request_send_http_header(VALUE self);
void rb_request_flush(VALUE request);
void ruby_init_apachelib();

#endif /* !APACHELIB_H */

/*
 * Local variables:
 * mode: C
 * tab-width: 8
 * End:
 */
