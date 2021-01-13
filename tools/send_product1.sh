#!/bin/sh


FACILITY_ID=9467629eddc215d0bc2db7e08abec4dc

mosquitto_pub -t unit1/${FACILITY_ID}/offline -m "HELLO" -q 1
