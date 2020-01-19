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
#include <stdbool.h>

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

typedef struct
{
    const char* appname;
    const char* logtarget;
    const char* logfile;
    const char* logfacility;
    const char* loglevel;
    const char* mqtt_broker_host;
    int         mqtt_broker_port;
    int         mqtt_keepalive;

    bool        mqtt_tls;
    const char* mqtt_cafile;
    const char* mqtt_capath;
    const char* mqtt_certfile;
    const char* mqtt_keyfile;

    bool        mqtt_user_pw;
    const char* mqtt_user;
    const char* mqtt_pw;

} Configuration;


/* This struct holds the configuration of 
 * one unit. It also has a reference to the common config items.
 * This struct passed to the worker threads
 */
typedef struct
{
    const char* unit_name;
    bool        enabled;
    const char* webservice_baseurl;
    const char* mqtt_topic;
    Configuration *common_configuration;
} UnitConfiguration;


Configuration *init_config(const char*filepath);
int get_unitconfigs(UnitConfiguration *configarray[],  const int max_size);
void free_config();

#endif
