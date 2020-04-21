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
#include "mqtt_curl.h"
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include <pthread.h>


#include <config.h>
#include "logging.h"
#include <sys/types.h>

#define MAX_TOPIC_LENGTH 128

int
rest_post(UnitConfiguration *config, const char *url, const char *payload )
{
    CURL *curl;
    CURLcode res;
    int retval = 0;
    const int len = strlen(config->webservice_baseurl);
    DEBUG("len url: %d", strlen(url));
    #define URL_MAX_SIZE 256
    char full_url[URL_MAX_SIZE];
    strncpy(full_url,config->webservice_baseurl, URL_MAX_SIZE);
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

/* Called when a message arrives to the subscribed topic,
 * we just removing the lead topic and turn it into an URL and calling
 * it, payload attached
 */
void
mqtt_cb_msg(struct mosquitto *mosq, void *userdata,
                  const struct mosquitto_message *msg)
{
    UnitConfiguration *config = (UnitConfiguration *)userdata;
    //tailoring the url
    char *url = msg->topic + strlen(config->mqtt_topic) + 1; // +1 the '/'
    DEBUG("Received msg on topic: %s\n", msg->topic);
    //calling the URL with the payload
    if(msg->payload != NULL){
        DEBUG("Payload: %s", msg->payload);
    }
    rest_post(config, url, msg->payload);
}


void
mqtt_cb_connect(struct mosquitto *mosq, void *userdata, int result)
{
    UnitConfiguration *c = (UnitConfiguration *)userdata;

    pid_t tid = pthread_self();
    DEBUG("MQTT connect, UNIT: %s, thread: %ld", c->unit_name, (long)tid);
    char buffer[MAX_TOPIC_LENGTH];
    if(snprintf(buffer,MAX_TOPIC_LENGTH,"%s/#",c->mqtt_topic) >= MAX_TOPIC_LENGTH){
        FATAL("Topic length in config is too long, the max is %d",MAX_TOPIC_LENGTH);
        return;
    }
    if(!result){
        mosquitto_subscribe(mosq, NULL, buffer, 2);
    }else{
        WARNING("MQTT Connect failed\n");
    }
}

void
mqtt_cb_subscribe(struct mosquitto *mosq, void *userdata, int mid,
                        int qos_count, const int *granted_qos)
{
    UnitConfiguration *c = (UnitConfiguration *)userdata;
    INFO("Unit [%s]: Subscribed (mid: %d): %d", c->unit_name, mid, granted_qos[0]);
    for(int i=1; i<qos_count; i++){
        INFO("\t %d", granted_qos[i]);
    }
}

void
mqtt_cb_disconnect(struct mosquitto *mosq, void *userdata, int rc)
{
    UnitConfiguration *c = (UnitConfiguration *)userdata;
    WARNING("Unit [%s] MQTT disconnect, error: %d: %s",c->unit_name, rc, mosquitto_strerror(rc));
}


/* transpose libmosquitto log messages into ours
 * MOSQ_LOG_INFO MOSQ_LOG_NOTICE MOSQ_LOG_WARNING MOSQ_LOG_ERR MOSQ_LOG_DEBUG
 */
void
mqtt_cb_log(struct mosquitto *mosq, void *userdata,
                  int level, const char *str)
{
    UnitConfiguration *c = (UnitConfiguration *)userdata;
    switch(level){
        case MOSQ_LOG_DEBUG:
            DEBUG("Unit [%s]: %s",c->unit_name, str);
            break;
        case MOSQ_LOG_INFO:
        case MOSQ_LOG_NOTICE:
            INFO("Unit [%s]: %s",c->unit_name, str);
            break;
        case MOSQ_LOG_WARNING:
            WARNING("Unit [%s]: %s",c->unit_name, str);
            break;
        case MOSQ_LOG_ERR:
            ERROR("Unit [%s]: %s",c->unit_name, str);
            break;
        default:
            FATAL("Unknown MOSQ loglevel!");
    }
}

struct mosquitto*
mqtt_curl_init(UnitConfiguration *config)
{
    struct mosquitto *mosq;
    bool clean_session = true;

    mosq = mosquitto_new(config->unit_name, clean_session, config);
    if(!mosq){
        FATAL("Error: Out of memory.\n");
        return NULL;
    }
    mosquitto_threaded_set(mosq,true);
    mosquitto_user_data_set(mosq,config);

    mosquitto_log_callback_set(mosq, mqtt_cb_log);
    mosquitto_connect_callback_set(mosq, mqtt_cb_connect);
    mosquitto_message_callback_set(mosq, mqtt_cb_msg);
    mosquitto_subscribe_callback_set(mosq, mqtt_cb_subscribe);
    mosquitto_disconnect_callback_set(mosq, mqtt_cb_disconnect);

    return mosq;
}

bool
mqtt_curl_connect(struct mosquitto *mosq, Configuration *config)
{
    int count = 3;
    bool retval = false;

    if (config->mqtt_tls){
        INFO("Enable MQTT TLS support");
        int r = mosquitto_tls_set(mosq,
            config->mqtt_cafile,
            config->mqtt_capath,
            config->mqtt_certfile,
            config->mqtt_keyfile,
            NULL);
        if (r != MOSQ_ERR_SUCCESS){
            FATAL("Failed to set up TLS, check config!");
        }
    }
    if (config->mqtt_user_pw){
        int r = mosquitto_username_pw_set(mosq,config->mqtt_user,config->mqtt_pw);
        if (r != MOSQ_ERR_SUCCESS){
            FATAL("Failed to set up username/password, check config!");
        }
    }

    while(count--){
        int ret = mosquitto_connect(mosq, config->mqtt_broker_host,
                             config->mqtt_broker_port, config->mqtt_keepalive);
        if(ret){
            ERROR("Unable to connect, host: %s, port: %d, error: %s",
                   config->mqtt_broker_host,
                   config->mqtt_broker_port,
                   mosquitto_strerror(ret));
            sleep(2);
        }
        else {
            retval = true;
            break;
        }
    }
    return retval;
}
