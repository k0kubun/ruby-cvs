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

#define MOD_RUBY_STRING_VERSION "mod_ruby/0.8.5"
#define RUBY_GATEWAY_INTERFACE "CGI-Ruby/1.1"

typedef struct {
    array_header *load_path;
    table *env;
    int timeout;
} ruby_server_config;

typedef struct {
    char *kcode;
    table *env;
    int safe_level;
    array_header *ruby_handler;
    array_header *ruby_trans_handler;
    array_header *ruby_authen_handler;
    array_header *ruby_authz_handler;
    array_header *ruby_access_handler;
    array_header *ruby_type_handler;
    array_header *ruby_fixup_handler;
    array_header *ruby_log_handler;
    array_header *ruby_header_parser_handler;
    array_header *ruby_post_read_request_handler;
    array_header *ruby_init_handler;
    array_header *ruby_cleanup_handler;
} ruby_dir_config;

extern MODULE_VAR_EXPORT module ruby_module;
extern array_header *ruby_required_libraries;

void ruby_log_error(server_rec *s, int state);
int ruby_running();
int ruby_require(char*);
void ruby_add_path(const char *path);
void rb_setup_cgi_env(request_rec *r);

#define get_server_config(s) \
	((ruby_server_config *) ap_get_module_config(s->module_config, \
						     &ruby_module))
#define get_dir_config(r) \
	((ruby_dir_config *) ap_get_module_config(r->per_dir_config, \
						  &ruby_module))

#endif /* !MOD_RUBY_H */

/*
 * Local variables:
 * mode: C
 * tab-width: 8
 * End:
 */
