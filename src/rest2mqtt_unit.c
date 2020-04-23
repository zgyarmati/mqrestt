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
#include "rest2mqtt_unit.h"
#include "configuration.h"
#include "logging.h"



/* Feel free to use this example code in any way
   you see fit (Public Domain) */

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include <microhttpd.h>
#include "mqtt_client.h"

// from main.c
extern bool running;

static void
request_completed (void *cls, struct MHD_Connection *connection,
        void **con_cls, enum MHD_RequestTerminationCode toe)
{
    (void)cls;         /* Unused. Silent compiler warning. */
    (void)connection;  /* Unused. Silent compiler warning. */
    (void)toe;         /* Unused. Silent compiler warning. */

    *con_cls = NULL;
}


static int
answer_to_connection (void *cls, struct MHD_Connection *connection,
        const char *url, const char *method,
        const char *version, const char *upload_data,
        size_t *upload_data_size, void **con_cls)
{
    (void)cls;               /* Unused. Silent compiler warning. */
    (void)url;               /* Unused. Silent compiler warning. */
    (void)version;           /* Unused. Silent compiler warning. */
    INFO("CONNECT, url: %s, type: %s, version: %s ", url, method,version);

    if (strcmp (method, "POST"))// we accept only POST
    {
        return MHD_NO;
    }

    struct MHD_PostProcessor *pp = *con_cls;
    if (pp == NULL)
    {
        INFO("GOT POST connect, data: %s",upload_data);
        *con_cls = (void*)&running;
        return MHD_YES;
    }
    if (*upload_data_size)
    {
        INFO("GOT POST continuation, data: %s",upload_data);
        *upload_data_size = 0;

        // todo move this to the else branch
        const char *qos_val = MHD_lookup_connection_value(connection,MHD_GET_ARGUMENT_KIND,"qos");
        INFO("QOS: %s",qos_val);
        int qos;
        parseInt(qos_val,&qos);
        mqtt_client_publish((struct MqttClientHandle*)cls, 
                url, upload_data,qos);
        return MHD_YES;
    }
    else
    {

        struct MHD_Response *response = MHD_create_response_from_buffer (3, "OK",
                MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header (response,MHD_HTTP_HEADER_CONTENT_TYPE, "text/html");
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }
}

void *rest2mqtt_unit_run(void *configdata)
{
    assert(configdata != NULL);
    Rest2MqttUnitConfiguration *unitconfig = (Rest2MqttUnitConfiguration*)configdata;
    Configuration *config = (Configuration *)unitconfig->common_configuration;
    assert(config != NULL);
    INFO("Starting REST->MQTT unit: %s", unitconfig->unit_name);
    //set up the mqtt client config
    MqttClientConfiguration mqtt_config;
    mqtt_config.label = unitconfig->unit_name;
    mqtt_config.topic = NULL;
    mqtt_config.broker_host = config->mqtt_broker_host;
    mqtt_config.broker_port = config->mqtt_broker_port;
    mqtt_config.keepalive = config->mqtt_keepalive;
    mqtt_config.tls_enabled = config->mqtt_tls;
    mqtt_config.cafile = config->mqtt_cafile;
    mqtt_config.capath = config->mqtt_capath;
    mqtt_config.certfile = config->mqtt_certfile;
    mqtt_config.keyfile = config->mqtt_keyfile;
    mqtt_config.user_pw_auth_enabled = config->mqtt_user_pw;
    mqtt_config.user = config->mqtt_user;
    mqtt_config.pw = config->mqtt_pw;
    mqtt_config.callback_context = NULL;
    mqtt_config.msg_callback = NULL;

    struct MqttClientHandle *mqtt = mqtt_client_init(&mqtt_config);
    if (mqtt == NULL){
        FATAL("Failed to init MQTT client");
        return NULL;
    }
    mqtt_client_connect(mqtt);
    struct pollfd pfd[1];
    nfds_t nfds = sizeof(pfd)/sizeof(struct pollfd);
    const int poll_timeout = config->mqtt_keepalive/2*1000;

    //microhttpd setup
    struct MHD_Daemon *daemon;
    daemon = MHD_start_daemon (MHD_USE_DEBUG,
            unitconfig->listen_port, 
            NULL, NULL,
            &answer_to_connection, (void*)mqtt,
            MHD_OPTION_NOTIFY_COMPLETED, request_completed,
            NULL, MHD_OPTION_END);
    while(running)
    {
        if (!mqtt_client_connected(mqtt))
        {
            DEBUG("Trying to reconnect...");
            if (!mqtt_client_reconnect(mqtt))
            {
                continue;
            }
        }

        mqtt_client_get_pollfds(mqtt,pfd,&nfds);
        int mosq_fd = pfd[0].fd; 
        // ToDo migrate to poll
        struct timeval tv;
        fd_set rs;
        fd_set ws;
        fd_set es;

        tv.tv_sec = 1;
        tv.tv_usec = 0;
        int max = 0;
        FD_ZERO (&rs);
        FD_ZERO (&ws);
        FD_ZERO (&es);
        if (MHD_YES != MHD_get_fdset (daemon, &rs, &ws, &es, &max))
        {
            FATAL("Failed to get MHD fdset");
        }
        if (mosq_fd > max) max = mosq_fd;
        FD_SET(mosq_fd,&rs);
        if (pfd[0].events & POLLOUT)
        {
            FD_SET(mosq_fd,&ws);
        }
        select (max + 1, &rs, &ws, &es, &tv);
        MHD_run (daemon);
        mqtt_client_loop(mqtt,FD_ISSET(mosq_fd,&rs),
                              FD_ISSET(mosq_fd,&ws));
    }
    MHD_stop_daemon (daemon);
    mqtt_client_destroy(mqtt);

    return 0;
}
