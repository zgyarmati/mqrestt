/*  file configuration.c
 *  author: Zoltan Gyarmati <mr.zoltan.gyarmati@gmail.com>
 */


#include "configuration.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include "iniparser/iniparser.h"

#define INISECTION PACKAGE_NAME":"


/*
 * parses the ini file, and returns a pointer to a
 * dynamically allocated instance of the
 * configuration struct with the config values
 * The caller supposed to free up the returned
 * object
 * If the return value is NULL, the parsing was unsuccesfull,
 * and the error supposed to be printed on stderr
 */
Configuration *
init_config(const char*filepath){
    Configuration *retval = malloc(sizeof(Configuration));
    dictionary  *ini ;
    ini = iniparser_load(filepath);
    if (ini==NULL) {
        fprintf(stderr, "cannot parse file: %s\n", filepath);
        return NULL;
    }
    iniparser_dump(ini, stderr);
    retval->appname = INISECTION;
    //logging
    retval->logtarget = iniparser_getstring(ini,INISECTION"logtarget","stdout");
    retval->logfile = iniparser_getstring(ini,INISECTION"logfile","timezoned.log");
    retval->logfacility = iniparser_getstring(ini,INISECTION"logfacility","local0");
    retval->loglevel = iniparser_getstring(ini,INISECTION"loglevel","fatal");
    //application
    //webservice
    retval->webservice_baseurl = iniparser_getstring(ini,INISECTION"webservice_baseurl","127.0.0.1");
    //MQTT
    retval->mqtt_broker_host = iniparser_getstring(ini,INISECTION"mqtt_broker_host","127.0.0.1");
    retval->mqtt_broker_port = iniparser_getint(ini,INISECTION"mqtt_broker_port",1883);

    retval->mqtt_topic = iniparser_getstring(ini,INISECTION"mqtt_topic","facility");
    return retval;
}

