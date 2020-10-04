AM_CFLAGS = -Wall -std=gnu99 ${libcurl_CFLAGS} $(PTHREAD_CFLAGS)
AM_LDFLAGS = -lm

bin_PROGRAMS = mqrestt
mqrestt_SOURCES = logging.c configuration.c main.c \
		  mqtt2rest_unit.c rest2mqtt_unit.c \
		  utils.c mqtt_client.c

mqrestt_LDADD =  ${libcurl_LIBS} ${MOSQUITTO_LIBS} @LIBCURL@  ${libconfuse_LIBS} \
				 $(PTHREAD_LDFLAGS) ${libmicrohttpd_LIBS}
