#!/bin/sh


curl -d '{"key1":"value123", "key2":"value2"}' -H "Content-Type: application/json" -X POST http://127.0.0.1:9000/data?qos=1  -v -v
