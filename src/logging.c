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
 *   Copyright  Zoltan Gyarmati <zgyarmati@zgyarmati.de> 2021
 */

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <math.h>

#define SYSLOG_NAMES
#include "logging.h"

#define LOG_MSG_MAX_LENGTH 256  // we dont want longer messages then 255 chars

#ifndef WIN32
#define TERM_MAGENTA  "\033[35m"
#define TERM_CYAN     "\033[36m"
#define TERM_RED     "\033[1;31m"
#define TERM_GREEN   "\033[1;32m"
#define TERM_NORMAL  "\033[0;39m"
#define TERM_BOLD    "\033[1m"
#else
#define TERM_MAGENTA
#define TERM_CYAN
#define TERM_RED
#define TERM_GREEN
#define TERM_NORMAL
#define TERM_BOLD
#endif

int loglevel = 0;
FILE *logfile = NULL;
char *logfilename;
void (*log_print)(log_msg_type, char*,int,const char *, const char *,...);

char* print_time(char *buf, size_t bufsize)
{
    int millisec;
    struct tm* tm_info;
    struct timeval tv;
    char temp[10];

    gettimeofday(&tv, NULL);
    millisec = tv.tv_usec/1000.0; // Round to nearest millisec
    if (millisec>=1000) { // Allow for rounding up to nearest second
        millisec -=1000;
        tv.tv_sec++;
    }

    tm_info = localtime(&tv.tv_sec);

    strftime(temp, 10, "%H:%M:%S", tm_info);
    snprintf(buf,bufsize, "%s.%03d", temp, millisec);
    return buf;
}

int log_init(const char *cloglevel, const char *target, const char *filepath, 
                 const char *facility, const long int max_size)
{
    loglevel = log_lookup_loglevel(cloglevel);
    logfilename = strdup(filepath);

    if (!strcmp(target,"stdout")){
        log_print = log_print_console;
    }
    else if (!strcmp(target,"syslog"))
    #ifndef WIN32
    {
        openlog("mqrestt", LOG_CONS | LOG_PID | LOG_NDELAY,
                log_lookup_syslog_facility(facility));
        log_print = log_print_syslog;
    }
    #else
        fprintf (stderr, "Syslog log target not supported on Windows");
    #endif
    else if (!strcmp(target,"file")){
        logfile = fopen (logfilename,"a+");
        if (!logfile){
            fprintf(stderr, "Opening logfile %s failed: %s\n", logfilename,strerror(errno));
            return -1;
        }
        log_print = log_print_file;
    }
    return 0;
}

void log_print_file (log_msg_type type, char*file,
                int line,
                const char *function,
                const char *formatstring,...)
{

    // let's not do anything if it's not needed
    if (type <= loglevel){
        va_list args;
        char buffer[LOG_MSG_MAX_LENGTH];  // we dont want longer messages than 256 chars
        int i=0;
        char timestring[16];
        va_start (args, formatstring);
        char *msg_head;
        switch (type){
        case log_fatal:
            msg_head = "FTL";
            break;
        case log_error:
            msg_head = "ERR";
            break;
        case log_warning:
            msg_head = "WRN";
            break;
        case log_info:
            msg_head = "NFO";
            break;
        case log_debug:
            msg_head = "DBG";
            break;
        default:
            msg_head = "UNSPECIFIED: ";
        }


        i = snprintf(buffer,LOG_MSG_MAX_LENGTH, "[%s][%s] (%s:%d,%s) ",
                      msg_head,print_time(timestring, sizeof(timestring)),file,line, function );
        i = vsnprintf(buffer+i, LOG_MSG_MAX_LENGTH-i-1,formatstring,args);
        va_end(args);

        if (logfile && (ftell(logfile) != -1)){
            fputs(buffer, logfile);
            fputc('\n', logfile);
            fflush(logfile);
        }
        else {
            fprintf(stderr, "Write to logfile failed: %s\n",
                    strerror(errno));
        }

    if (type == log_fatal)
        exit (1);

    }//if (type >= loglevel)
}

void log_print_console (log_msg_type type, char*file,
                int line,
                const char *function,
                const char *formatstring,...)
{

    // let's not do anything if it's not needed
    if (type <= loglevel){
        va_list args;
        char buffer[LOG_MSG_MAX_LENGTH];  // we dont want longer messages than 256 chars
        int i=0;
        char timestring[16];
        va_start (args, formatstring);
        char *msg_head;
        switch (type){
        case log_fatal:
            msg_head = TERM_RED"FTL"TERM_NORMAL;
            break;
        case log_error:
            msg_head = TERM_RED"ERR"TERM_NORMAL;
            break;
        case log_warning:
            msg_head = TERM_CYAN"WRN"TERM_NORMAL;
            break;
        case log_info:
            msg_head =TERM_GREEN"NFO"TERM_NORMAL;
            break;
        case log_debug:
            msg_head = TERM_BOLD"DBG"TERM_NORMAL;
            break;
        default:
            msg_head = "UNSPECIFIED: ";
        }


        i = snprintf(buffer,LOG_MSG_MAX_LENGTH, "[%s][%s] (%s:%d,%s) ",
                      msg_head,print_time(timestring, sizeof(timestring)),file,line, function );
        i = vsnprintf(buffer+i, LOG_MSG_MAX_LENGTH-i-1,formatstring,args);
        va_end(args);

        puts(buffer);
//        putchar('\n');
    }//if (type >= loglevel)

    if (type == log_fatal)
        exit (1);
}

void log_print_syslog (log_msg_type type, char*file,
                int line,
                const char *function,
                const char *formatstring,...)
{
#ifndef WIN32
    // let's not do anything if it's not needed
    if (type <= loglevel){
        va_list args;
        va_start (args, formatstring);
        switch (type){
        case log_fatal:
            syslog(LOG_EMERG,formatstring, args);
            break;
        case log_error:
            syslog(LOG_CRIT,formatstring, args);
            break;
        case log_warning:
            syslog(LOG_WARNING,formatstring, args);
            break;
        case log_info:
            syslog(LOG_INFO,formatstring, args);
            break;
        case log_debug:
            syslog(LOG_DEBUG,formatstring, args);
            break;
        default:
            break;
        }

        va_end(args);


    }//if (type >= loglevel)
#else
    fprintf (stderr, "Syslog log target not supported on Windows");
#endif

    if (type == log_fatal)
        exit (1);
}
void log_finalize(){

     if (logfile){
        fclose(logfile);
     }
     logfile = NULL;
}

#ifndef WIN32
int
log_lookup_syslog_facility(const char *name){
    int retval = LOG_LOCAL0;
    int i = 0;
    while(facilitynames[i].c_name){
        if (!strcmp(name,facilitynames[i].c_name)){
            printf("found facility: %s, code: %d\n",
            facilitynames[i].c_name, facilitynames[i].c_val);
            retval = facilitynames[i].c_val;
            break;
        }
        i++;
    }
    return retval;
}
#endif


int
log_lookup_loglevel(const char *name){
    int retval = 0;
    if (!strcmp(name,"fatal")){
        retval = 0;
    }
    else if (!strcmp(name,"error")){
        retval = 1;
    }
    else if (!strcmp(name,"warning")){
        retval = 2;
    }
    else if (!strcmp(name,"info")){
        retval = 3;
    }
    else if (!strcmp(name,"debug")){
        retval = 4;
    }
    else {
        fprintf(stderr, "Loglevel not found %s\n:", name);
    }

    return retval;
}
