/*! @file configuration.h
 *  author: Zoltan Gyarmati <mr.zoltan.gyarmati@gmail.com>
 *
 *                   The MIT License (MIT)
 *
 * Copyright (c) 2016 Zoltan Gyarmati <mr.zoltan.gyarmati@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of 
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so, 
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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
    const char* webservice_baseurl;
    const char* mqtt_broker_host;
    int         mqtt_broker_port;
    const char* mqtt_topic;
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


Configuration *init_config(const char*filepath);

#endif
