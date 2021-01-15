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
#include "rest2mqtt_unit.h"
#include "configuration.h"
#include "logging.h"

/* Feel free to use this example code in any way
   you see fit (Public Domain) */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "mqtt_client.h"
#include "utils.h"
#include <microhttpd.h>

typedef struct IncomingData {
    size_t length;
    char *data;
} IncomingData;

// as MHD doesn't have API to get pollfds, we use this function
// to  convert the select fd_sets to pollfd
nfds_t get_mhd_pollfds(struct MHD_Daemon *h, struct pollfd *pfds, nfds_t *count)
{
    int max = 0;
    fd_set rs, ws, es;
    FD_ZERO(&rs);
    FD_ZERO(&ws);
    FD_ZERO(&es);
    if (MHD_YES != MHD_get_fdset(h, &rs, &ws, &es, &max)) {
        FATAL("Failed to get MHD fdset");
        return 0;
    }
    nfds_t pollfd_counter = 0;
    for (int fd = 0; fd <= max; fd++) {
        bool r_set = FD_ISSET(fd, &rs);
        bool w_set = FD_ISSET(fd, &ws);
        bool e_set = FD_ISSET(fd, &es);
        if (r_set || w_set || e_set) {
            pfds[pollfd_counter].fd = fd;
            pfds[pollfd_counter].events = 0;
            if (r_set)
                pfds[pollfd_counter].events |= POLLIN;
            if (w_set)
                pfds[pollfd_counter].events |= POLLOUT;
            if (e_set)
                pfds[pollfd_counter].events |= POLLPRI;
            if (++pollfd_counter >= *count) {
                FATAL("Not enough pollfd-s allocated for libmicrohttpd!");
                return *count;
            }
        }
    }
    *count = pollfd_counter;
    return *count;
}

static int answer_to_connection(void *cls, struct MHD_Connection *connection,
                                const char *url, const char *method,
                                const char *version, const char *upload_data,
                                size_t *upload_data_size, void **con_cls)
{
    (void)cls;     /* Unused. Silent compiler warning. */
    (void)url;     /* Unused. Silent compiler warning. */
    (void)version; /* Unused. Silent compiler warning. */
    INFO("CONNECT, url: %s, type: %s, version: %s ", url, method, version);

    if (strcmp(method, "POST")) // we accept only POST
    {
        return MHD_NO;
    }

    if (!*con_cls) {
        INFO("GOT POST connect, data: %s %zd", upload_data, *upload_data_size);
        IncomingData *incoming = SAFEMALLOC(sizeof(IncomingData));
        incoming->length = 0;
        incoming->data = NULL;
        *con_cls = (void *)incoming;
        return MHD_YES;
    }
    if (*upload_data_size) {
        INFO("GOT POST continuation, size %zd,  data: %s", *upload_data_size,
             upload_data);
        IncomingData *incoming = *con_cls;
        incoming->data =
            SAFEREALLOC(incoming->data, incoming->length + *upload_data_size);
        incoming->data[incoming->length] = '\0';
        strncat(incoming->data + incoming->length, upload_data,
                *upload_data_size);
        incoming->length += *upload_data_size;
        *upload_data_size = 0;

        return MHD_YES;
    } else {
        IncomingData *incoming = *con_cls;
        const char *qos_val = MHD_lookup_connection_value(
            connection, MHD_GET_ARGUMENT_KIND, "qos");
        INFO("QOS: %s", qos_val);
        int qos;
        parseInt(qos_val, &qos);
        mqtt_client_publish((struct MqttClientHandle *)cls, url, incoming->data,
                            qos);

        struct MHD_Response *response =
            MHD_create_response_from_buffer(3, "OK", MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE,
                                "text/html");
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        free(incoming->data);
        free(incoming);
        return ret;
    }
}

void *rest2mqtt_unit_run(void *configdata)
{
    assert(configdata != NULL);
    Rest2MqttUnitConfiguration *unitconfig =
        (Rest2MqttUnitConfiguration *)configdata;
    Configuration *config = (Configuration *)unitconfig->common_configuration;
    assert(config != NULL);
    INFO("Starting REST->MQTT unit: %s", unitconfig->unit_name);
    // set up the mqtt client config
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
    if (mqtt == NULL) {
        FATAL("Failed to init MQTT client");
        return NULL;
    }
    mqtt_client_connect(mqtt);
    const int poll_timeout = config->mqtt_keepalive / 2 * 1000;

    // microhttpd setup
    struct MHD_Daemon *daemon;
    daemon =
        MHD_start_daemon(MHD_USE_DEBUG, unitconfig->listen_port, NULL, NULL,
                         &answer_to_connection, (void *)mqtt, MHD_OPTION_END);
    while (true) {
        if (!mqtt_client_connected(mqtt)) {
            DEBUG("Trying to reconnect...");
            if (!mqtt_client_reconnect(mqtt)) {
                continue;
            }
        }
        struct pollfd pfd[64];
        nfds_t nfds = sizeof(pfd) / sizeof(struct pollfd);
        nfds_t pfd_count = get_mhd_pollfds(daemon, pfd, &nfds);

        struct pollfd mqtt_pfd[1];
        nfds_t mqtt_pfd_size = 1;
        mqtt_client_get_pollfds(mqtt, mqtt_pfd, &mqtt_pfd_size);
        pfd[pfd_count++] = mqtt_pfd[0];
        const int ret = poll(pfd, pfd_count, poll_timeout);
        if (ret < 0) {
            if (errno != EINTR) // we got SIGUSR1 so we halt
            {
                ERROR("Poll() failed with <%s>, exiting", strerror(errno));
            }
            break;
        }
        MHD_run(daemon);
        mqtt_client_loop(mqtt, pfd[pfd_count - 1].revents & POLLIN,
                         pfd[pfd_count - 1].revents & POLLOUT);
    }
    MHD_stop_daemon(daemon);
    mqtt_client_destroy(mqtt);
    INFO("Unit thread %s exiting...", unitconfig->unit_name);

    return NULL;
}
