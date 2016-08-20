AM_CFLAGS = -Wall -std=gnu99 ${libcurl_CFLAGS}
AM_LDFLAGS = -lm

bin_PROGRAMS = mqrestt
mqrestt_SOURCES = logging.c configuration.c configuration.h \
					iniparser/dictionary.c iniparser/iniparser.c \
					iniparser/dictionary.h  iniparser/iniparser.h \
					logging.h main.c

mqrestt_LDADD =  ${libcurl_LIBS} ${MOSQUITTO_LIBS} @LIBCURL@
