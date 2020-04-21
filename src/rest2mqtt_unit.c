/*
 *  This file is part of mqrestt.
 *
 *  Mqrestt is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Mqrestt is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with mqrestt.  If not, see <https://www.gnu.org/licenses/>.
 *
 *   Copyright  Zoltan Gyarmati <zgyarmati@zgyarmati.de> 2020
 */
#include "rest2mqtt_unit.h"
#include "configuration.h"
#include "logging.h"



/* Feel free to use this example code in any way
   you see fit (Public Domain) */

#include <sys/types.h>
#ifndef _WIN32
#include <sys/select.h>
#include <sys/socket.h>
#else
#include <winsock2.h>
#endif
#include <microhttpd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#if defined(_MSC_VER) && _MSC_VER+0 <= 1800
/* Substitution is OK while return value is not used */
#define snprintf _snprintf
#endif

#define PORT            8888
#define POSTBUFFERSIZE  1024
#define MAXNAMESIZE     20
#define MAXANSWERSIZE   512

#define GET             0
#define POST            1

// from main.c
extern bool running;

static void
request_completed (void *cls, struct MHD_Connection *connection,
                   void **con_cls, enum MHD_RequestTerminationCode toe)
{
  (void)cls;         /* Unused. Silent compiler warning. */
  (void)connection;  /* Unused. Silent compiler warning. */
  (void)toe;         /* Unused. Silent compiler warning. */

  *con_cls = NULL;
}


static int
answer_to_connection (void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      size_t *upload_data_size, void **con_cls)
{
  (void)cls;               /* Unused. Silent compiler warning. */
  (void)url;               /* Unused. Silent compiler warning. */
  (void)version;           /* Unused. Silent compiler warning. */
  INFO("CONNECT, url: %s, type: %s, version: %s ", url, method,version);

  if (strcmp (method, "POST"))// we accept only POST
  {
    return MHD_NO;
  }

    struct MHD_PostProcessor *pp = *con_cls;
    if (pp == NULL)
    {
      INFO("GOT POST connect, data: %s",upload_data);
      *con_cls = (void*)&running;
      return MHD_YES;
    }
    if (*upload_data_size)
    {
      INFO("GOT POST continuation, data: %s",upload_data);
      *upload_data_size = 0;
      return MHD_YES;
    }
    else
    {
      const char *qos_val = MHD_lookup_connection_value(connection,MHD_GET_ARGUMENT_KIND,"qos");
      INFO("QOS: %s",qos_val);

      struct MHD_Response *response = MHD_create_response_from_buffer (3, "OK",
                                            MHD_RESPMEM_PERSISTENT);
      MHD_add_response_header (response,MHD_HTTP_HEADER_CONTENT_TYPE, "text/html");
      int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
      MHD_destroy_response(response);
      return ret;
    }
}

void *rest2mqtt_unit_run(void *configdata)
{
    struct MHD_Daemon *daemon;

    daemon = MHD_start_daemon (MHD_USE_AUTO | MHD_USE_INTERNAL_POLLING_THREAD, PORT, NULL, NULL,
                             &answer_to_connection, NULL,
                             MHD_OPTION_NOTIFY_COMPLETED, request_completed,
                             NULL, MHD_OPTION_END);
    while(running)  sleep(1); //ToDo figure out how to run MHD with own event loop
    MHD_stop_daemon (daemon);
    return 0;
}
