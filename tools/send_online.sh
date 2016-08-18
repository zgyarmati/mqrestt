#!/bin/sh


FACILITY_ID=9467629eddc215d0bc2db7e08abec4dc

mosquitto_pub -t facility/online -m  ${FACILITY_ID} -q 1
