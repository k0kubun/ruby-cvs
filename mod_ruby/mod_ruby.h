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

#ifndef MOD_RUBY_H
#define MOD_RUBY_H

#define MOD_RUBY_STRING_VERSION "mod_ruby/0.8.4"
#define RUBY_GATEWAY_INTERFACE "CGI-Ruby/1.1"

typedef struct {
    array_header *load_path;
    array_header *required_files;
    table *env;
    int timeout;
} ruby_server_config;

typedef struct {
    char *kcode;
    table *env;
    int safe_level;
    array_header *ruby_handlers;
} ruby_dir_config;

extern MODULE_VAR_EXPORT module ruby_module;
int ruby_running();
int ruby_require(char*);
void ruby_add_path(const char *path);
void rb_setup_cgi_env(request_rec *r);

#endif /* !MOD_RUBY_H */

/*
 * Local variables:
 * mode: C
 * tab-width: 8
 * End:
 */
