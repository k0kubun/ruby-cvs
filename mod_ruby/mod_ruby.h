/*
 * $Id$
 * Copyright (C) 2000  ZetaBITS, Inc.
 * Copyright (C) 2000  Information-technology Promotion Agency, Japan
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

#ifndef MOD_RUBY_H
#define MOD_RUBY_H

#define MOD_RUBY_STRING_VERSION "mod_ruby/0.9.7"
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
    int output_mode;
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

#define MR_DEFAULT_TIMEOUT 270
#define MR_DEFAULT_SAFE_LEVEL 1

#define MR_OUTPUT_DEFAULT	0
#define MR_OUTPUT_NOSYNC	1
#define MR_OUTPUT_SYNC		2
#define MR_OUTPUT_SYNC_HEADER	3

extern MODULE_VAR_EXPORT module ruby_module;
extern array_header *ruby_required_libraries;

VALUE rb_protect_funcall(VALUE recv, ID mid, int *state, int argc, ...);
void ruby_log_error(server_rec *s, VALUE errmsg);
VALUE ruby_get_error_info(int state);
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
