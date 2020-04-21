/*
 *  This file is part of mqrestt.
 *
 *  Mqrestt is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Mqrestt is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with mqrestt.  If not, see <https://www.gnu.org/licenses/>.
 *
 *   Copyright  Zoltan Gyarmati <zgyarmati@zgyarmati.de> 2020
 */


#include "configuration.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

#include <config.h>
#include <confuse.h>
#include <assert.h>

#include "logging.h"

#define INISECTION PACKAGE_NAME":"


cfg_t *cfg = NULL;;
/*
 * parses the config file, and returns a pointer to a
 * dynamically allocated instance of the
 * configuration struct with the config values
 * The caller supposed to free up the returned
 * object
 * If the return value is NULL, the parsing was unsuccesfull,
 * and the error supposed to be printed on stderr
 */
Configuration *
init_config(const char*filepath)
{
    Configuration *retval = malloc(sizeof(Configuration));

    static cfg_opt_t unit_opts[] =
    {
        CFG_STR("webservice_baseurl", "localhost", CFGF_NONE),
        CFG_STR("mqtt_topic", "default_topic", CFGF_NONE),
        CFG_BOOL("enabled", true, CFGF_NONE),
        CFG_END()
    };

    static cfg_opt_t rest2mqtt_unit_opts[] =
    {
        CFG_INT("listen_port", 8888, CFGF_NONE),
        CFG_STR("mqtt_topic_root", "default_topic", CFGF_NONE),
        CFG_BOOL("enabled", true, CFGF_NONE),
        CFG_END()
    };

    cfg_opt_t opts[] = {
        //logging
        CFG_STR("logtarget", "stdout", CFGF_NONE),
        CFG_STR("logfile", "mgrestt.log", CFGF_NONE),
        CFG_STR("logfacility", "local0", CFGF_NONE),
        CFG_STR("loglevel", "fatal", CFGF_NONE),
        // top level mqtt broker options
        CFG_STR("mqtt_broker_host", "localhost", CFGF_NONE),
        CFG_INT("mqtt_broker_port", 1883, CFGF_NONE),

        CFG_INT("mqtt_keepalive", 30, CFGF_NONE),
        CFG_BOOL("mqtt_tls", false, CFGF_NONE),
        CFG_STR("mqtt_cafile", "-----", CFGF_NONE),
        CFG_STR("mqtt_capath", "-----", CFGF_NONE),
        CFG_STR("mqtt_certfile", "-----", CFGF_NONE),
        CFG_STR("mqtt_keyfile", "-----", CFGF_NONE),

        CFG_BOOL("mqtt_user_pw", false, CFGF_NONE),
        CFG_STR("mqtt_user", "-----", CFGF_NONE),
        CFG_STR("mqtt_pw", "-----", CFGF_NONE),

        CFG_SEC("unit", unit_opts, CFGF_MULTI | CFGF_TITLE),
        CFG_SEC("rest2mqtt_unit", rest2mqtt_unit_opts, CFGF_MULTI | CFGF_TITLE),
        CFG_END()
    };


    cfg = cfg_init(opts, 0);
    cfg_parse(cfg, filepath);


    if (cfg==NULL) {
        fprintf(stderr, "cannot parse file: %s\n", filepath);
        free_config();
        return NULL;
    }
    cfg_print(cfg, stderr);
    retval->appname = INISECTION;
    //logging
    retval->logtarget = cfg_getstr(cfg,"logtarget");
    retval->logfile = cfg_getstr(cfg,"logfile");
    retval->logfacility = cfg_getstr(cfg,"logfacility");
    retval->loglevel = cfg_getstr(cfg,"loglevel");
    //application
   
    //MQTT
    retval->mqtt_broker_host = cfg_getstr(cfg,"mqtt_broker_host");
    retval->mqtt_broker_port = cfg_getint(cfg,"mqtt_broker_port");
    retval->mqtt_keepalive = cfg_getint(cfg,"mqtt_keepalive");

    retval->mqtt_tls = cfg_getbool(cfg,"mqtt_tls");
    retval->mqtt_cafile = cfg_getstr(cfg,"mqtt_cafile");
    retval->mqtt_capath = cfg_getstr(cfg,"mqtt_capath");
    retval->mqtt_certfile = cfg_getstr(cfg,"mqtt_certfile");
    retval->mqtt_keyfile = cfg_getstr(cfg,"mqtt_keyfile");

    if(retval->mqtt_cafile && !strlen(retval->mqtt_cafile)){
        retval->mqtt_cafile = NULL;
    }

    if(retval->mqtt_capath && !strlen(retval->mqtt_capath)){
        retval->mqtt_capath = NULL;
    }

    if(retval->mqtt_certfile && !strlen(retval->mqtt_certfile)){
        retval->mqtt_certfile = NULL;
    }

    if(retval->mqtt_keyfile && !strlen(retval->mqtt_keyfile)){
        retval->mqtt_keyfile = NULL;
    }

    if(retval->mqtt_tls &&
       !retval->mqtt_cafile &&
       !retval->mqtt_capath) {
        fprintf(stderr,"config error: either mqtt_cafile or mqtt_capath need to be set!\n");
        free_config();
        return NULL;
    }
    retval->mqtt_user_pw = cfg_getbool(cfg,"mqtt_user_pw");

    retval->mqtt_user = cfg_getstr(cfg,"mqtt_user");
    retval->mqtt_pw = cfg_getstr(cfg,"mqtt_pw");
    return retval;
}

int
get_unitconfigs(UnitConfiguration *configarray[], const int max_size)
{
    assert(cfg != NULL);
    const int unit_count = cfg_size(cfg,"unit");
    if (max_size < unit_count)
    {
        fprintf(stderr,"config error: configarray too small\n");
        return -1;
    }
    for(int i = 0; i < unit_count; i++)
    {
        configarray[i] = malloc (sizeof(UnitConfiguration));
        if (configarray[i] == NULL)
        {
            fprintf(stderr,"failed to allocate memory\n");
            return -1;
        }
        cfg_t *unit = cfg_getnsec(cfg, "unit", i);

        INFO("UNIT: %s", cfg_title(unit));
        configarray[i]->unit_name = cfg_title(unit);

        INFO("\tURL: %s", cfg_getstr(unit, "webservice_baseurl"));
        configarray[i]->webservice_baseurl = cfg_getstr(unit, "webservice_baseurl");

        INFO("\tTOPIC: %s", cfg_getstr(unit, "mqtt_topic"));
        configarray[i]->mqtt_topic = cfg_getstr(unit, "mqtt_topic");
        configarray[i]->enabled = cfg_getbool(unit,"enabled");
    }
    return unit_count;
}

get_rest2mqtt_unitconfigs(Rest2MqttUnitConfiguration *configarray[],
                          const int max_size)
{
    assert(cfg != NULL);
    const int unit_count = cfg_size(cfg,"rest2mqtt_unit");
    if (max_size < unit_count)
    {
        fprintf(stderr,"config error: configarray too small\n");
        return -1;
    }
    for(int i = 0; i < unit_count; i++)
    {
        configarray[i] = malloc (sizeof(Rest2MqttUnitConfiguration));
        if (configarray[i] == NULL)
        {
            fprintf(stderr,"failed to allocate memory\n");
            return -1;
        }
        cfg_t *unit = cfg_getnsec(cfg, "rest2mqtt_unit", i);

        INFO("UNIT: %s", cfg_title(unit));
        configarray[i]->unit_name = cfg_title(unit);

        INFO("\tListen port: %d", cfg_getint(unit, "listen_port"));
        configarray[i]->listen_port = cfg_getint(unit, "listen_port");

        configarray[i]->mqtt_topic_root = cfg_getstr(unit, "mqtt_topic_root");
        INFO("\tTOPIC ROOT: %s", configarray[i]->mqtt_topic_root);
        configarray[i]->enabled = cfg_getbool(unit,"enabled");
    }
}

void free_config()
{
    assert(cfg != NULL);
    cfg_free(cfg);
}
