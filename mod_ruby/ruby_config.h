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

#ifndef RUBY_CONFIG_H
#define RUBY_CONFIG_H

void *ruby_create_server_config(pool*, server_rec*);
void *ruby_merge_dir_config(pool*, void*, void*);
void *ruby_create_dir_config (pool*, char*);
const char *ruby_cmd_kanji_code(cmd_parms*, ruby_dir_config*, char*);
const char *ruby_cmd_add_path(cmd_parms*, void*, char*);
const char *ruby_cmd_require(cmd_parms*, void*, char*);
const char *ruby_cmd_pass_env(cmd_parms*, void*, char*);
const char *ruby_cmd_set_env(cmd_parms*, ruby_dir_config*, char*, char*);
const char *ruby_cmd_timeout(cmd_parms*, void*, char*);
const char *ruby_cmd_safe_level(cmd_parms*, void*, char*);
const char *ruby_cmd_handler(cmd_parms*, ruby_dir_config*, char*);

#endif /* !RUBY_CONFIG_H */

/*
 * Local variables:
 * mode: C
 * tab-width: 8
 * End:
 */
