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
 *   Copyright  Zoltan Gyarmati <zgyarmati@zgyarmati.de> 2021
 */

#include "mqtt_client.h"
#include <assert.h>

#include <errno.h>
#include <sys/poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <curl/curl.h>
#include <sys/types.h>
#include <string.h>

#include "configuration.h"
#include "logging.h"

#define URL_MAX_SIZE 2048

static int
rest_post(Mqtt2RestUnitConfiguration *config, const char *url, 
          const char *payload )
{
    CURL *curl;
    CURLcode res;
    int retval = 0;
    const int len = strlen(config->webservice_baseurl);
    DEBUG("len url: %d", strlen(url));
    char full_url[URL_MAX_SIZE];
    strncpy(full_url,config->webservice_baseurl, URL_MAX_SIZE-1);
    if(full_url[len-1] != '/'){
        DEBUG("Need to append a '/'");
        full_url[len] = '/';
        full_url[len + 1] = '\0';
    }
    strncat(full_url,url,URL_MAX_SIZE-len-1);
    INFO("FULL URL: %s", full_url);

    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, full_url);
        if (payload == NULL){
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
        }
        else {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
        }
        res = curl_easy_perform(curl);
        if(res != CURLE_OK){
            ERROR("curl_easy_perform() failed: %s", curl_easy_strerror(res));
            retval = -1;
        }
        curl_easy_cleanup(curl);
    }
    return retval;
}

void
on_mqtt_msg(const char* topic, const char* msg, void *ctx)
{
    INFO("Got MQTT msg on topic %s",topic);
    Mqtt2RestUnitConfiguration *unitconfig = (Mqtt2RestUnitConfiguration*)ctx;
    //tailoring the url, removing the base topic from the beggining by advancing the pointer
    const char *url = topic + strlen(unitconfig->mqtt_topic) + 1; // +1 the '/'
    //calling the URL with the payload
    if(msg != NULL)
    {
        DEBUG("Payload: %s", msg);
    }
    rest_post(unitconfig, url, msg);
}


void *mqtt2rest_unit_run(void *configdata)
{
    assert(configdata != NULL);
    Mqtt2RestUnitConfiguration *unitconfig = (Mqtt2RestUnitConfiguration*)configdata;
    INFO("Starting thread for unit: %s", unitconfig->unit_name);
    Configuration *config = (Configuration *)unitconfig->common_configuration;
    assert(config != NULL);

    //set up the mqtt client config
    MqttClientConfiguration mqtt_config;
    mqtt_config.label = unitconfig->unit_name;
    mqtt_config.topic = unitconfig->mqtt_topic;
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
    mqtt_config.callback_context = (void*)unitconfig;
    mqtt_config.msg_callback = &on_mqtt_msg;

    struct MqttClientHandle *mqtt = mqtt_client_init(&mqtt_config);
    if (mqtt == NULL){
        FATAL("Failed to init MQTT client");
        return NULL;
    }
    mqtt_client_connect(mqtt);
    struct pollfd pfd[1];
    nfds_t nfds = sizeof(pfd)/sizeof(struct pollfd);
    const int poll_timeout = config->mqtt_keepalive/2*1000;

    while (true)
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
        const int ret = poll(pfd, nfds, poll_timeout);
        if(ret < 0)
        {
            if (errno != EINTR)  // we got SIGUSR1 so we halt
            {
                ERROR("Poll() failed with <%s>, exiting",strerror(errno));
            }
            break;
        }
        mqtt_client_loop(mqtt,pfd[0].revents & POLLIN,
                              pfd[0].revents & POLLOUT);
    }


    INFO("Unit thread %s exiting...", unitconfig->unit_name);
    return NULL;
}
