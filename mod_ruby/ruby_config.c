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
#include "http_request.h"
#include "util_script.h"
#include "multithread.h"

#include "mod_ruby.h"
#include "ruby_config.h"

#define MOD_RUBY_DEFAULT_TIMEOUT 270
#define MOD_RUBY_DEFAULT_SAFE_LEVEL 1

void *ruby_create_server_config(pool *p, server_rec *s)
{
    ruby_server_config *conf =
	(ruby_server_config *) ap_pcalloc(p, sizeof(ruby_server_config));

    conf->load_path = ap_make_array(p, 1, sizeof(char*));
    conf->required_files = ap_make_array(p, 1, sizeof(char*));
    conf->env = ap_make_table(p, 1);
    conf->timeout = MOD_RUBY_DEFAULT_TIMEOUT;
    return conf;
}

void *ruby_create_dir_config (pool *p, char *dirname)
{
    ruby_dir_config *conf =
	(ruby_dir_config *) ap_palloc(p, sizeof (ruby_dir_config));

    conf->kcode = NULL;
    conf->env = ap_make_table(p, 5); 
    conf->safe_level = MOD_RUBY_DEFAULT_SAFE_LEVEL;
    conf->handlers = ap_make_array(p, 1, sizeof(char*));
    return conf;
}

void *ruby_merge_dir_config(pool *p, void *basev, void *addv)
{
    ruby_dir_config *new =
	(ruby_dir_config *) ap_pcalloc(p, sizeof(ruby_dir_config));
    ruby_dir_config *base = (ruby_dir_config *) basev;
    ruby_dir_config *add = (ruby_dir_config *) addv;

    new->kcode = add->kcode ? add->kcode : base->kcode;
    new->env = ap_overlay_tables(p, add->env, base->env);
    if (add->safe_level > base->safe_level) {
	new->safe_level = add->safe_level;
    }
    else {
	new->safe_level = base->safe_level;
    }
    new->handlers =
	ap_append_arrays(p, add->handlers, base->handlers);
    return (void *) new;
}

const char *ruby_cmd_kanji_code(cmd_parms *cmd, ruby_dir_config *conf, char *arg)
{
    conf->kcode = ap_pstrdup(cmd->pool, arg);
    return NULL;
}

const char *ruby_cmd_add_path(cmd_parms *cmd, void *dummy, char *arg)
{
    ruby_server_config *conf =
	(ruby_server_config *) ap_get_module_config(cmd->server->module_config,
						    &ruby_module);

    if (ruby_running()) {
	ruby_add_path(arg);
    }
    else {
	*(char **) ap_push_array(conf->load_path) =
	    ap_pstrdup(cmd->pool, arg);
    }
    return NULL;
}

const char *ruby_cmd_require(cmd_parms *cmd, void *dummy, char *arg)
{
    ruby_server_config *conf =
	(ruby_server_config *) ap_get_module_config(cmd->server->module_config,
						    &ruby_module);

    if (ruby_running()) {
	ruby_require(arg);
    }
    else {
	*(char **) ap_push_array(conf->required_files) =
	    ap_pstrdup(cmd->pool, arg);
    }
    return NULL;
}

const char *ruby_cmd_pass_env(cmd_parms *cmd, void *dummy, char *arg)
{
    ruby_server_config *conf =
	(ruby_server_config *) ap_get_module_config(cmd->server->module_config,
						    &ruby_module);
    char *key;
    char *val = strchr(arg, ':');

    if (val) {
	key = ap_pstrndup(cmd->pool, arg, val - arg);
	val++;
    }
    else {
	key = arg;
	val = getenv(key);
    }
    ap_table_set(conf->env, key, val);
    return NULL;
}

const char *ruby_cmd_set_env(cmd_parms *cmd, ruby_dir_config *conf,
			     char *key, char *val)
{
    ap_table_set(conf->env, key, val);
    if (cmd->path == NULL) {
	ruby_server_config *sconf =
	    (ruby_server_config *) ap_get_module_config(cmd->server->module_config,
							&ruby_module);
	ap_table_set(sconf->env, key, val);
    }
    return NULL;
}

const char *ruby_cmd_timeout(cmd_parms *cmd, void *dummy, char *arg)
{
    ruby_server_config *conf =
	(ruby_server_config *) ap_get_module_config(cmd->server->module_config,
						    &ruby_module);

    conf->timeout = atoi(arg);
    return NULL;
}

const char *ruby_cmd_safe_level(cmd_parms *cmd, ruby_dir_config *conf, char *arg)
{
    conf->safe_level = atoi(arg);
    return NULL;
}

const char *ruby_cmd_handler(cmd_parms *cmd, ruby_dir_config *conf, char *arg)
{
    *(char **) ap_push_array(conf->handlers) =
	ap_pstrdup(cmd->pool, arg);
    return NULL;
}

/*
 * Local variables:
 * mode: C
 * tab-width: 8
 * End:
 */
