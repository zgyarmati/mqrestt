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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <getopt.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <math.h>
#include <assert.h>
#include <sys/poll.h>
#include <pthread.h>

#include <curl/curl.h>
#include <mosquitto.h>

#include <config.h>
#include "configuration.h"
#include "logging.h"
#include "mqtt2rest_unit.h"
#include "rest2mqtt_unit.h"



volatile bool running = true;
static char *conf_file_name = PACKAGE_NAME".conf";
static char *pid_file = "/var/lock/"PACKAGE_NAME;
static int  pid_fd = -1;
static char *app_name = PACKAGE_NAME;


Configuration *config = NULL;
#define MAX_UNIT_NUM  32




/**
 * \brief   Callback function for handling signals.
 * \param	sig identifier of signal
 */
void handle_signal(int sig)
{
    if(sig == SIGINT) {
     //   fprintf(log_stream, "Debug: stopping daemon ...\n");
        /* Unlock and close lockfile */
        if(pid_fd != -1) {
            if(!lockf(pid_fd, F_ULOCK, 0)){
                fprintf(stderr,"Failed to unlock lockfile\n");
            }
            close(pid_fd);
        }
        /* Try to delete lockfile */
        if(pid_file != NULL) {
            unlink(pid_file);
        }
        // notify the unit threads to exit
        running = false;
    }
}


/**
 * \brief This function will daemonize this app
 */
static void daemonize()
{
    pid_t pid = 0;
    int fd;

    /* Fork off the parent process */
    pid = fork();

    /* An error occurred */
    if(pid < 0) {
        exit(EXIT_FAILURE);
    }

    /* Success: Let the parent terminate */
    if(pid > 0) {
        exit(EXIT_SUCCESS);
    }

    /* On success: The child process becomes session leader */
    if(setsid() < 0) {
        exit(EXIT_FAILURE);
    }

    /* Ignore signal sent from child to parent process */
    signal(SIGCHLD, SIG_IGN);

    /* Fork off for the second time*/
    pid = fork();

    /* An error occurred */
    if(pid < 0) {
        exit(EXIT_FAILURE);
    }

    /* Success: Let the parent terminate */
    if(pid > 0) {
        exit(EXIT_SUCCESS);
    }


    /* Set new file permissions */
    umask(0);

    /* Change the working directory to the root directory */
    /* or another appropriated directory */
    if(chdir("/")){
        fprintf(stderr,"Failed to cd to /, exiting");
        exit(-1);
    }

    /* Close all open file descriptors */
    for(fd = sysconf(_SC_OPEN_MAX); fd > 0; fd--)
    {
        close(fd);
    }

    /* Reopen stdin (fd = 0), stdout (fd = 1), stderr (fd = 2) */
    stdin = fopen("/dev/null", "r");
    stdout = fopen("/dev/null", "w+");
    stderr = fopen("/dev/null", "w+");

    /* Try to write PID of daemon to lockfile */
    if(pid_file!= NULL)
    {
        char str[256];
        pid_fd = open(pid_file, O_RDWR|O_CREAT, 0640);
        if(pid_fd < 0)
        {
            /* Can't open lockfile */
            exit(EXIT_FAILURE);
        }
        if(lockf(pid_fd, F_TLOCK, 0) < 0)
        {
            /* Can't lock file */
            exit(EXIT_FAILURE);
        }
        /* Get current PID */
        sprintf(str, "%d\n", getpid());
        /* Write PID to lockfile */
        if(!(write(pid_fd, str, strlen(str)))){
            fprintf(stderr,"Failed to writen pid into lockfile, exiting...");
            exit(-1);
        }
    }
}

/**
 * \brief Print help for this application
 */
void
print_help(void)
{
    fprintf(stderr,"\nUsage: %s [OPTIONS]\n\n", app_name);
    fprintf(stderr,"Options:\n");
    fprintf(stderr,"   -h --help                 Print this help\n");
    fprintf(stderr,"   -v --version              Print version info\n");
    fprintf(stderr,"   -c --configfile filename  Read configuration from the file\n");
    fprintf(stderr,"   -d --daemon               Daemonize this application\n");
    fprintf(stderr,"   -p --dump                 Dump the parsed config to stderr and continue running\n");
    fprintf(stderr,"\n");
}

void
print_version(void)
{
    int major;
    int minor;
    int revision;
    fprintf(stderr,PACKAGE_NAME" version: %s\n", PACKAGE_VERSION);
    int mosquitto_ver = mosquitto_lib_version(&major,&minor,&revision);
    fprintf(stderr, "Libmosquitto version: %d.%d-%d (%d)\n",
            major, minor, revision, mosquitto_ver);
    fprintf(stderr, "Libcurl version: %s\n", curl_version());
}





int main(int argc, char *argv[])
{
    static struct option long_options[] = {
        {"configfile", required_argument, 0, 'c'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {"daemon", no_argument, 0, 'd'},
        {"dump", no_argument, 0, 'p'},
        {NULL, 0, 0, 0}
    };
    int value, option_index = 0;
    int start_daemonized = 0;
    bool dump_config = false;

    app_name = argv[0];

    /* Try to process all command line arguments */
    while( (value = getopt_long(argc, argv, "c:dhvp", long_options, &option_index)) != -1) {
        switch(value) {
            case 'c':
                conf_file_name = strdup(optarg);
                break;
            case 'd':
                start_daemonized = 1;
                break;
            case 'p':
                dump_config = true;
                break;
            case 'h':
                print_help();
                return EXIT_SUCCESS;
            case 'v':
                print_version();
                return EXIT_SUCCESS;
            case '?':
                print_help();
                return EXIT_FAILURE;
            default:
                break;
        }
    }

    signal(SIGINT, handle_signal);

    /* When daemonizing is requested at command line. */
    if(start_daemonized == 1) {
        daemonize();
    }
    config = init_config(conf_file_name, dump_config);
    if (config == NULL){
        fprintf(stderr, "CONFIG ERROR in %s, EXITING!\n", conf_file_name);
        exit(EXIT_FAILURE);
    }
    if(0 != log_init(config->loglevel, config->logtarget, config->logfile,
              config->logfacility, 0)){
        fprintf(stderr, "Failed to init logging from config file: %s, exiting\n",
                        conf_file_name);
        return EXIT_FAILURE;
    }
    INFO(PACKAGE_NAME" started, pid: %d, config file: %s", getpid(),conf_file_name);

    // we need to call this only once, and it's not thread
    // safe, so we do it here
    mosquitto_lib_init();

    // setting up storage for the unit configuration list
    int mqtt2rest_count = MAX_UNIT_NUM;
    Mqtt2RestUnitConfiguration *unit_configs[MAX_UNIT_NUM] = {NULL};
    mqtt2rest_count = get_mqtt2rest_unitconfigs(unit_configs, mqtt2rest_count);

    int rest2mqtt_count = MAX_UNIT_NUM;
    Rest2MqttUnitConfiguration *rest2mqtt_unit_configs[MAX_UNIT_NUM] = {NULL};
    rest2mqtt_count = get_rest2mqtt_unitconfigs(rest2mqtt_unit_configs, rest2mqtt_count);

    if (mqtt2rest_count < 0 || rest2mqtt_count < 0)
    {
        FATAL("Failed to init unit configs, check config file:",conf_file_name);
        return EXIT_FAILURE;
    }
    if ((mqtt2rest_count + rest2mqtt_count) == 0)
    {
        ERROR("No units found. Please check configuration file %s",conf_file_name);
        return EXIT_FAILURE;
    }

    // create a thread for each mqtt2rest unit
    pthread_t threads[mqtt2rest_count];
    int threadcounter = 0;
    for (int i=0;i<mqtt2rest_count;i++)
    {
        if (!unit_configs[i]->enabled){
            continue;
        }
        // setting the common config in each unit configuration
        unit_configs[i]->common_configuration = config;
        int ret = pthread_create(&threads[threadcounter++], NULL, mqtt2rest_unit_run, (void*) unit_configs[i]);
        if(ret) {
            fprintf(stderr,"Error - pthread_create() return code: %d\n",ret);
            exit(EXIT_FAILURE);
        }
    }


    // create a thread for each rest2mqtt unit
    for (int i=0;i<rest2mqtt_count;i++)
    {
        if (!rest2mqtt_unit_configs[i]->enabled)
        {
            continue;
        }
        rest2mqtt_unit_configs[i]->common_configuration = config;
        int ret = pthread_create(&threads[threadcounter++], NULL, rest2mqtt_unit_run, (void*) rest2mqtt_unit_configs[i]);
        if(ret) {
            fprintf(stderr,"Error - pthread_create() return code: %d\n",ret);
            exit(EXIT_FAILURE);
        }
    }
    DEBUG("Started %d units",threadcounter);

    // waiting for all of the threads to exit, if ever
    for (int i = 0; i < threadcounter; i++) {
        pthread_join(threads[i], 0);
    }

    // freeing up the per-unit configs
    for (int i=0; i < mqtt2rest_count; i++)
    {
        free(unit_configs[i]);
    }
    for (int i=0; i < rest2mqtt_count; i++)
    {
        free(rest2mqtt_unit_configs[i]);
    }
    // free up the main config
    free_config();
    mosquitto_lib_cleanup();
    log_finalize();
    INFO("bye");
    return EXIT_SUCCESS;
}
