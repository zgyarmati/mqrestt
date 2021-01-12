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
 *
 *   @file mqtt_client.h
 *   @brief The mqtt_client unit provides an abstraction
 *   layer over the used mqtt client implementation, and contains
 *   the common functionality of the mqtt client. It has its
 *   own config struct which can be filled from the global config.
 */
#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/poll.h>



typedef struct
{
    const char* label;
    const char* topic;
    const char* broker_host;
    int         broker_port;
    int         keepalive;

    bool        tls_enabled;
    const char* cafile;
    const char* capath;
    const char* certfile;
    const char* keyfile;

    bool        user_pw_auth_enabled;
    const char* user;
    const char* pw;
    void *callback_context;
    void (*msg_callback)(const char* topic, const char* msg, void *ctx);

} MqttClientConfiguration;

struct MqttClientHandle;

struct MqttClientHandle *mqtt_client_init(MqttClientConfiguration *config);
bool mqtt_client_connect(struct MqttClientHandle *h);
bool mqtt_client_connected(struct MqttClientHandle *h);
bool mqtt_client_reconnect(struct MqttClientHandle *h);
bool mqtt_client_publish(struct MqttClientHandle *h, const char *topic, const char *msg,int qos);
nfds_t mqtt_client_get_pollfds(struct MqttClientHandle *h, struct pollfd *pfds, nfds_t *count);
void mqtt_client_loop(struct MqttClientHandle *h,const bool read, const bool write);
void mqtt_client_destroy(struct MqttClientHandle *h);

#endif
