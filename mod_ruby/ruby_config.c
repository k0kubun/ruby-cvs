/*
 * $Id$
 * Copyright (C) 1998-1999  Network Applied Communication Laboratory, Inc.
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

#include "ruby_module.h"
#include "ruby_config.h"

#define MR_DEFAULT_TIMEOUT 299

void *ruby_create_server_config(pool *p, server_rec *s)
{
    ruby_server_config *conf =
	(ruby_server_config *) ap_pcalloc(p, sizeof(ruby_server_config));

    conf->required_files = ap_make_array(p, 1, sizeof(char*));
    conf->env = ap_make_table(p, 1);
    conf->timeout = MR_DEFAULT_TIMEOUT;
    return conf;
}

void *ruby_create_dir_config (pool *p, char *dirname)
{
    ruby_dir_config *conf =
	(ruby_dir_config *) ap_palloc(p, sizeof (ruby_dir_config));

    conf->kcode = NULL;
    conf->env = ap_make_table(p, 5); 
    return conf;
}

void *ruby_merge_dir_config(pool *p, void *parent_conf, void *newloc_conf)
{
    ruby_dir_config *merged_conf =
	(ruby_dir_config *) ap_pcalloc(p, sizeof(ruby_dir_config));
    ruby_dir_config *pconf = (ruby_dir_config *) parent_conf;
    ruby_dir_config *nconf = (ruby_dir_config *) newloc_conf;

    merged_conf->kcode = ap_pstrdup(p, nconf->kcode);
    merged_conf->env = ap_overlay_tables(p, nconf->env, pconf->env);
    return (void *) merged_conf;
}

const char *ruby_cmd_kanji_code(cmd_parms *cmd, ruby_dir_config *conf, char *arg)
{
    conf->kcode = ap_pstrdup(cmd->pool, arg);
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

const char *ruby_cmd_timeout(cmd_parms *cmd, void *config, char *arg)
{
    ruby_server_config *conf =
	(ruby_server_config *) ap_get_module_config(cmd->server->module_config,
						    &ruby_module);

    conf->timeout = atoi(arg);
    return NULL;
}

/*
 * Local variables:
 * mode: C
 * tab-width: 8
 * End:
 */
