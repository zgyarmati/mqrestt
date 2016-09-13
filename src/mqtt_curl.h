#ifndef MQTT_CURL_H
#define MQTT_CURL_H

#include <mosquitto.h>
#include "configuration.h"

struct mosquitto* mqtt_curl_init(Configuration *config);

#endif
