% mqrestt(1)

NAME
----
**mqrestt** - daemon to translate MQTT messages to REST POST calls
          and vica versa


SYNOPSIS
--------
**mqrestt** ['OPTIONS']


DESCRIPTION
-----------

 MQRESTT is a daemon/service which can either subscribe to certain topics 
 on the configured MQTT broker and translate received MQTT messages into 
 REST POST calls or acting as a simple HTTP(s) server, and publishing the 
 incoming POST requests via MQTT.
 In the MQTT->REST case the URI of the POST call is fabricated from the 
 MQTT topic the message was received on, and the body is the MQTT message
 itself.
 In the REST->MQTT case similarly, the MQTT topic is fabricated based on 
 the URI of the incoming request.
 The 2 use-case can run simultaneously, and can have multiple units
 configured, therefore it's possible to subscribe to 2 different topics
 and forward the messages to different web applications living on different 
 hosts.
 
 



OPTIONS
-------
-c --configfile /path/to/mqrestt.conf::
    Set the path to the configfile. Default is /etc/mqrestt.conf

-h --help::
    Print help text

-v --version::
    Print version info



CONFIGURATION
-------------

 See the commented /etc/mqrestt.conf for examples


ENVIRONMENT
-----------

**MQRESTT_CONFIG**

  Path to the configfile. If -c --configfile argument given as well, 
then the command line argument has higher priority.

BUGS
----
 
 See GitLab Issues: <https://gitlab.com/820labs/mqrestt/issues>


AUTHORS
-------
Zoltan Gyarmati <zgyarmati@zgyarmati.de>


COPYING
-------

 Copyright (c) 2021 Zoltan Gyarmati (https://zgyarmati.de)
 License: GPLv3
