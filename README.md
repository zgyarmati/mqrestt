mqrestt
===============

MQtt REST Translator, a MQTT<->REST bridge dameon



Use-cases
===========
The simplest configuration is to subscribe to one MQTT topic, and forward the received 
messages to the API server, as shown here:

               MQTT pub                 
               '/devices/12/temperature'      MQTT sub         HTTP POST                                     
     .--------------.                         '/'              'https://myapp.com/api/devices/12/temperature'
     | Sensor Nodes |           .-------------.     .---------.            .------------------.
     '--------------'---------->| MQTT broker |---->| MQresTT |----------->| Web API server   |
     '--------------'           '-------------'     '---------'            '------------------'
     '--------------'

In this case, MQresTT is configured to subscribe to all of the MQTT messages in all topics 
(as '/'), and the messages are forwarded to the base URI 'https://myapp.com/api', which 
base URI is extended with the MQTT topic of the message. The payload of the HTTP POST 
is the content of the MQTT message as is. See the sample mqrestt.conf to implement this 
without any auth or SSL enabled at [doc/simple_mqtt2rest__mqrestt.conf](doc/simple_mqtt2rest__mqrestt.conf)




ROADMAP
=======
0.1:
    - MQTT->REST support
    - REST->MQTT support

0.2
    - SSL support both MQTT and HTTPs
    - rewrite support, to rewrite MQTT topics into URL's and vica verse (a'la Apache mod_rewrite)


Bookmarks
========
https://iot.stackexchange.com/questions/3763/restapi-and-mqtt-broker

https://mosquitto.org/api/files/mosquitto-h.html


Hacking
========

Development dependencies (on Debian/Ubuntu): autoconf-archive libcurl4-openssl-dev libmosquitto-dev libconfuse-dev libconfuse-dev


