
#include "mqtt_curl.h"
#include <assert.h>

#include <errno.h>
#include <sys/poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "configuration.h"
#include "logging.h"
extern bool running;

void *mqrestt_unit_run(void *configdata)
{
    assert(configdata != NULL);
    UnitConfiguration *unitconfig = (UnitConfiguration*)configdata;
    INFO("Starting thread for unit: %s", unitconfig->unit_name);
    Configuration *config = (Configuration *)unitconfig->common_configuration;
    assert(config != NULL);

    //set up mosquitto
    struct mosquitto *mosq;
    mosq = mqtt_curl_init(unitconfig);
    if (mosq == NULL){
        FATAL("Failed to init MQTT client");
        return NULL;
    }
    bool mqtt_connected = mqtt_curl_connect(mosq, config);
    //pfd[0] is for the mosquitto socket, pfd[1] is for the mq file descriptor
    struct pollfd pfd[1];
    const int nfds = sizeof(pfd)/sizeof(struct pollfd);
    const int poll_timeout = config->mqtt_keepalive/2*1000;

    while (running) {
        if (!mqtt_connected){
            DEBUG("Trying to reconnect...");
            if (mosquitto_reconnect(mosq) == MOSQ_ERR_SUCCESS){
                mqtt_connected = true;
            }
        }
        int mosq_fd = mosquitto_socket(mosq); //this might change?
        pfd[0].fd = mosq_fd;
        pfd[0].events = POLLIN;
        if (mosquitto_want_write(mosq)){
            //printf("Set POLLOUT\n");
            pfd[0].events |= POLLOUT;
        }
        if(poll(pfd, nfds, poll_timeout) < 0) {
            FATAL("Poll() failed with <%s>, exiting",strerror(errno));
            return NULL;
        }
        // first checking the mosquitto socket
        if(pfd[0].revents & POLLOUT) {
            mosquitto_loop_write(mosq,1);
        }
        if(pfd[0].revents & POLLIN){
            int ret = mosquitto_loop_read(mosq, 1);
            if (ret == MOSQ_ERR_CONN_LOST ||
                ret == MOSQ_ERR_NO_CONN) {
                mqtt_connected = false;
            }
        }
        mosquitto_loop_misc(mosq);
    }


    mosquitto_destroy(mosq);
    DEBUG("Unit thread %s exiting...\n", unitconfig->unit_name);
    return NULL;
}
