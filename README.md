mqrestt
===============

MQtt REST Translator, a MQTT<->REST bridge dameon. The main aim of this daemon to
act as a glue layer between your IOT devices (communicating via MQTT), and your
web application, on a general way, meaning that the mqrestt deamon logic doesn't
have to know too much neither about your MQTT topics and neither about your 
webapplication's URIs. To price to pay for this is that you have some 
correlation  between your MQTT topics and your URIs, which is 
probably the sensible thing to do in any system using both MQTT and REST.
 The working principle is to forward MQTT messages as HTTP POST 
request to a web application, and vice versa, acting as a simple 
HTTP server, which receives POST requests, and publish the messages 
via MQTT. 
 In the MQTT->REST case the URI of the POST call is fabricated from the 
MQTT topic the message was received on, and the body is the MQTT message
itself.
 In the REST->MQTT case similarly, the MQTT topic is fabricated based on 
the URI of the incoming request.
 The 2 use-case can run simultaneously, and each can have multiple units
configured, therefore it's possible to subscribe to 2 different topics
and forward the messages to different web applications living on different 
hosts. Therefore it's also possible to have different projects/products 
connecting to the same MQTT broker, and running one instance of mqrestt, 
but 2 web applications managing the different networks.

Implementation
==============

mqrestt is implemented in C (C99 standard), in order to be able to run 
low-level platforms such as SOHO routers running OpenWrt. mqrestt depends
heavily on a number of libraries to implement its funcionality, namely
`libmosquitto`, `libconfuse`, `libmicrohttpd` and `libcurl`. All of these 
dependencies are well-known open source libraries, available for most of 
the OS-s and hardware platforms. The main target OS is Linux, but it 
should be easy to get it run on any OS, as long as the mentioned 
dependencies are available.



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


