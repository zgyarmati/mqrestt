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

#ifndef LOGGING_H
#define LOGGING_H

#ifndef WIN32
#include <syslog.h>
#endif
#include <string.h>



/* the members indicating the type/severity of the log message
 */
typedef enum {log_fatal, log_error, log_warning, log_info, log_debug} log_msg_type;


extern void (*log_print)(log_msg_type, char*,int,const char *, const char *,...);


/*
 * The DEBUG macro is used to print normal debug messages.
 */
#define DEBUG(...) log_print (log_debug, \
        __FILE__, __LINE__, __PRETTY_FUNCTION__, \
        __VA_ARGS__)
/*
 * The LOG_INFO macro is used to print info messages
 */
#define INFO(...) log_print (log_info, \
        __FILE__, __LINE__, __PRETTY_FUNCTION__, \
        __VA_ARGS__)

/*
 * The WARNING macro is used to print warnings.
 */
#define WARNING(...) log_print (log_warning, \
       __FILE__,__LINE__, __PRETTY_FUNCTION__, \
        __VA_ARGS__)
/*
 * The L_ERROR macro is used to print errors.
 */
#define ERROR(...) log_print (log_error, \
        __FILE__, __LINE__, __PRETTY_FUNCTION__, \
        __VA_ARGS__)
/*
 * The FATAL macro is used to print fatal erros,
 * and also quits the application.
 */
#define FATAL(...) log_print (log_fatal, \
        __FILE__, __LINE__, __PRETTY_FUNCTION__, \
        __VA_ARGS__)




/* This function is the workhorse under the debug macros above.
 * Don't call it from outside, as it might change....
 */
void log_print_file (
        log_msg_type type,
        char *file,
        int  line,
        const char *function,
        const char *formatstring,
        ...);

void log_print_console (
        log_msg_type type,
        char *file,
        int  line,
        const char *function,
        const char *formatstring,
        ...);

void log_print_syslog (
        log_msg_type type,
        char *file,
        int  line,
        const char *function,
        const char *formatstring,
        ...);

/*  init logging functionality
 *  @param file: the file to log
 *  @param level: the max loglevel
 */
int log_init(const char *loglevel, const char *target, const char *filepath,
             const char *facility, const long int max_size);

void log_finalize();
int log_lookup_loglevel(const char *name);
#ifndef WIN32
int log_lookup_syslog_facility(const char *name);
#endif

#endif
