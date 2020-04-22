#!/bin/sh


curl --output - -w "%{http_code}" -d '{"key1":"value123", "key2":"value2"}' -H "Content-Type: application/json" -X POST http://localhost:9000/data?qos=2  -v -v
