#include "mqtt_client.h"
#include <mosquitto.h>
#include "logging.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#define MAX_TOPIC_LENGTH 256

typedef struct MqttClientHandle
{
    struct mosquitto *mosq;
    MqttClientConfiguration *config;
    bool connected;
} MqttClientHandle;
/* Called when a message arrives to the subscribed topic,
 * we just removing the lead topic and turn it into an URL and calling
 * it, payload attached
 */
static void
mqtt_cb_msg(struct mosquitto *mosq, void *userdata,
                  const struct mosquitto_message *msg)
{
    MqttClientConfiguration *config = userdata;
    assert(config != NULL);
    if (config->msg_callback)
    {
        config->msg_callback(msg->topic,msg->payload,config->callback_context);
    }

#if 0
#endif
}

static void
mqtt_cb_subscribe(struct mosquitto *mosq, void *userdata, int mid,
                        int qos_count, const int *granted_qos)
{
    MqttClientConfiguration *config = userdata;
    assert(config != NULL);
    INFO("Unit [%s]: Subscribed to topic [%s] (mid: %d): %d",
         config->label, config->topic, mid, granted_qos[0]);
    for(int i=1; i<qos_count; i++)
    {
        INFO("\t %d", granted_qos[i]);
    }
}

static void
mqtt_cb_disconnect(struct mosquitto *mosq, void *userdata, int rc)
{
    MqttClientConfiguration *config = userdata;
    WARNING("Unit [%s] MQTT disconnect, error: %d: %s",config->label, rc, mosquitto_strerror(rc));
}

/* transpose libmosquitto log messages into ours
 * MOSQ_LOG_INFO MOSQ_LOG_NOTICE MOSQ_LOG_WARNING MOSQ_LOG_ERR MOSQ_LOG_DEBUG
 */
static void
mqtt_cb_log(struct mosquitto *mosq, void *userdata,
                  int level, const char *str)
{
    MqttClientConfiguration *config = userdata;
    assert(config != NULL);

    switch(level){
        case MOSQ_LOG_DEBUG:
            DEBUG("Unit [%s]: %s",config->label, str);
            break;
        case MOSQ_LOG_INFO:
        case MOSQ_LOG_NOTICE:
            INFO("Unit [%s]: %s",config->label, str);
            break;
        case MOSQ_LOG_WARNING:
            WARNING("Unit [%s]: %s",config->label, str);
            break;
        case MOSQ_LOG_ERR:
            ERROR("Unit [%s]: %s",config->label, str);
            break;
        default:
            FATAL("Unknown MOSQ loglevel!");
    }
}

static void
mqtt_cb_connect(struct mosquitto *mosq, void *userdata, int result)
{
    MqttClientConfiguration *config = userdata;
    assert(config != NULL);

    DEBUG("MQTT connect, UNIT: %s", config->label);
    if (!config->topic)
    {
        return;
    }
    char buffer[MAX_TOPIC_LENGTH];
    if(snprintf(buffer,MAX_TOPIC_LENGTH,"%s/#",config->topic) >= MAX_TOPIC_LENGTH)
    {
        FATAL("Topic length in config is too long, the max is %d",MAX_TOPIC_LENGTH);
        return;
    }
    if(!result){
        mosquitto_subscribe(mosq, NULL, buffer, 2);
    }else{
        WARNING("MQTT Connect failed\n");
    }
}

MqttClientHandle *
mqtt_client_init(MqttClientConfiguration *config)
{
    bool clean_session = true;
    MqttClientHandle *retval = malloc(sizeof(MqttClientHandle));
    struct mosquitto *mosq = mosquitto_new(config->label, clean_session, config);
    if(!mosq)
    {
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

    retval->mosq = mosq;
    retval->config = config;
    retval->connected = false;
    return retval;
}

bool
mqtt_client_connect(MqttClientHandle *h)
{
    assert(h->mosq != NULL);
    assert(h->config != NULL);

    int count = 3;

    if (h->config->tls_enabled)
    {
        INFO("Enable MQTT TLS support");
        int r = mosquitto_tls_set(h->mosq,
            h->config->cafile,
            h->config->capath,
            h->config->certfile,
            h->config->keyfile,
            NULL);
        if (r != MOSQ_ERR_SUCCESS)
        {
            FATAL("Failed to set up TLS, check config!");
        }
    }
    if (h->config->user_pw_auth_enabled)
    {
        int r = mosquitto_username_pw_set(h->mosq,h->config->user,
                                               h->config->pw);
        if (r != MOSQ_ERR_SUCCESS)
        {
            FATAL("Failed to set up username/password, check config!");
        }
    }

    while(count--)
    {
        int ret = mosquitto_connect(h->mosq, h->config->broker_host,
                             h->config->broker_port, h->config->keepalive);
        if(ret)
        {
            ERROR("Unable to connect, host: %s, port: %d, error: %s",
                   h->config->broker_host,
                   h->config->broker_port,
                   mosquitto_strerror(ret));
            sleep(2);
        }
        else
        {
            h->connected = true;
            break;
        }
    }
    return h->connected;
}
bool
mqtt_client_connected(MqttClientHandle *h)
{
    assert(h != NULL);
    return h->connected;
}

bool
mqtt_client_reconnect(MqttClientHandle *h)
{
    assert(h != NULL);
    return true;
}

bool
mqtt_client_publish(struct MqttClientHandle *h, const char* topic, const char *msg,int qos)
{
    INFO("Publishing on topic %s",topic+1);
    assert(h != NULL);
    int ret = mosquitto_publish(h->mosq,NULL,topic+1,strlen(msg),
                     (void*)msg,qos,false);
    if (ret != MOSQ_ERR_SUCCESS){
        WARNING("Failed to publish, reason:  %s",mosquitto_strerror(ret));
    }
    return (ret == MOSQ_ERR_SUCCESS);
}

nfds_t
mqtt_client_get_pollfds(MqttClientHandle *h, 
                        struct pollfd *pfds, 
                        nfds_t *count)
{
    assert(h != NULL);
    assert(pfds != NULL);
    assert(count > 0);
    pfds[0].fd = mosquitto_socket(h->mosq);
    pfds[0].events = POLLIN;
    if (mosquitto_want_write(h->mosq))
    {
        //printf("Set POLLOUT\n");
        pfds[0].events |= POLLOUT;
    }
    return 1;
}

void
mqtt_client_loop(MqttClientHandle *h,const bool read, const bool write)
{
    assert(h != NULL);
    int ret = MOSQ_ERR_SUCCESS;
    if(write)
    {
        ret = mosquitto_loop_write(h->mosq,1);
    }
    if(read)
    {
        ret = mosquitto_loop_read(h->mosq, 1);
    }

    if (ret == MOSQ_ERR_CONN_LOST ||
        ret == MOSQ_ERR_NO_CONN)
    {
        h->connected = false;
    }
    mosquitto_loop_misc(h->mosq);
}

void
mqtt_client_destroy(MqttClientHandle *h)
{
    assert(h != NULL);
    mosquitto_destroy(h->mosq);
    free(h);
}
