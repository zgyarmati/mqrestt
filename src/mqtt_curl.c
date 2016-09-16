#include "mqtt_curl.h"
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>

#include <config.h>
#include "logging.h"


int
rest_post(Configuration *config, const char *url, const char *payload )
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
    Configuration *config = (Configuration *)userdata;
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
    if(!result){
        mosquitto_subscribe(mosq, NULL, "facility/#", 2);
    }else{
        WARNING("MQTT Connect failed\n");
    }
}

void
mqtt_cb_subscribe(struct mosquitto *mosq, void *userdata, int mid,
                        int qos_count, const int *granted_qos)
{
    INFO("Subscribed (mid: %d): %d", mid, granted_qos[0]);
    for(int i=1; i<qos_count; i++){
        INFO("\t %d", granted_qos[i]);
    }
}

void
mqtt_cb_disconnect(struct mosquitto *mosq, void *userdat, int rc)
{
    WARNING("MQTT disconnect, error: %d: %s",rc, mosquitto_strerror(rc));
}


/* transpose libmosquitto log messages into ours
 * MOSQ_LOG_INFO MOSQ_LOG_NOTICE MOSQ_LOG_WARNING MOSQ_LOG_ERR MOSQ_LOG_DEBUG
 */
void
mqtt_cb_log(struct mosquitto *mosq, void *userdata,
                  int level, const char *str)
{
    switch(level){
        case MOSQ_LOG_DEBUG:
            DEBUG(str);
            break;
        case MOSQ_LOG_INFO:
        case MOSQ_LOG_NOTICE:
            INFO(str);
            break;
        case MOSQ_LOG_WARNING:
            WARNING(str);
            break;
        case MOSQ_LOG_ERR:
            ERROR(str);
            break;
        default:
            FATAL("Unknown MOSQ loglevel!");
    }
}

struct mosquitto*
mqtt_curl_init(Configuration *config)
{
    bool clean_session = true;
    struct mosquitto *mosq = NULL;

    mosquitto_lib_init();
    mosq = mosquitto_new(NULL, clean_session, config);
    if(!mosq){
        fprintf(stderr, "Error: Out of memory.\n");
        return NULL;
    }
    mosquitto_log_callback_set(mosq, mqtt_cb_log);
    mosquitto_connect_callback_set(mosq, mqtt_cb_connect);
    mosquitto_message_callback_set(mosq, mqtt_cb_msg);
    mosquitto_subscribe_callback_set(mosq, mqtt_cb_subscribe);
    mosquitto_disconnect_callback_set(mosq, mqtt_cb_disconnect);

    int running = 1; // changed from signal handler
    while(running){ //we try until we succeed, or we killed
        if(mosquitto_connect(mosq, config->mqtt_broker_host,
                             config->mqtt_broker_port, config->mqtt_keepalive)){
            ERROR("Unable to connect, host: %s, port: %d\n",
                   config->mqtt_broker_host, config->mqtt_broker_port);
            sleep(2);
            continue;
        }
        break;
    }
    return mosq;
}

