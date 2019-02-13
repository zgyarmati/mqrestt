AM_CFLAGS = -Wall -std=gnu99 ${libcurl_CFLAGS} $(PTHREAD_CFLAGS)
AM_LDFLAGS = -lm

bin_PROGRAMS = mqrestt
mqrestt_SOURCES = logging.c configuration.c configuration.h \
					logging.h main.c mqtt_curl.c mqtt_curl.h \
					mqrestt_unit.h mqrestt_unit.c

mqrestt_LDADD =  ${libcurl_LIBS} ${MOSQUITTO_LIBS} @LIBCURL@  ${libconfuse_LIBS} $(PTHREAD_LDFLAGS)
