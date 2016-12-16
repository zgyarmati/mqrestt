/*  file main.c
 *  author: Zoltan Gyarmati <mr.zoltan.gyarmati@gmail.com>
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

#include <curl/curl.h>
#include <mosquitto.h>

#include <config.h>
#include "configuration.h"
#include "logging.h"
#include "mqtt_curl.h"



static int  running = 0;
static char *conf_file_name = PACKAGE_NAME".ini";
static char *pid_file = "/var/lock/"PACKAGE_NAME;
static int  pid_fd = -1;
static char *app_name = PACKAGE_NAME;


Configuration *config = NULL;




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
        running = 0;
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
    fprintf(stderr, "Libcurl version info: %s\n", curl_version());
}





int main(int argc, char *argv[])
{
    static struct option long_options[] = {
        {"configfile", required_argument, 0, 'c'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {"daemon", no_argument, 0, 'd'},
        {NULL, 0, 0, 0}
    };
    int value, option_index = 0;
    int start_daemonized = 0;

    app_name = argv[0];

    /* Try to process all command line arguments */
    while( (value = getopt_long(argc, argv, "c:dhv", long_options, &option_index)) != -1) {
        switch(value) {
            case 'c':
                conf_file_name = strdup(optarg);
                break;
            case 'd':
                start_daemonized = 1;
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

    /* When daemonizing is requested at command line. */
    if(start_daemonized == 1) {
        daemonize();
    }
    config = init_config(conf_file_name);
    if (config == NULL){
        fprintf(stderr, "CONFIG ERROR, EXITING!\n");
        exit(EXIT_FAILURE);
    }
    if(0 != log_init(config->loglevel, config->logtarget, config->logfile,
              config->logfacility, 0)){
        fprintf(stderr, "Failed to init logging, exiting\n");
        return EXIT_FAILURE;
    }
    INFO(PACKAGE_NAME" started, pid: %d", getpid());
    //set up mosquitto
    struct mosquitto *mosq = NULL;
    mosq = mqtt_curl_init(config);
    bool mqtt_connected = mqtt_curl_connect(mosq, config);
    if (mosq == NULL){
        ERROR("Failed to init MQTT client");
    }
    running = 1;
    //pfd[0] is for the mosquitto socket, pfd[1] is for the mq file descriptor
    struct pollfd pfd[1];
    const int nfds = sizeof(pfd)/sizeof(struct pollfd);
    const int poll_timeout = config->mqtt_keepalive/2*1000;

    while (running) {
        if (!mqtt_connected){
            DEBUG("Trying to reconnect...");
            if (mosquitto_reconnect(mosq) == MOSQ_ERR_SUCCESS){
                mqtt_connected = true;
            }
        }
        int mosq_fd = mosquitto_socket(mosq); //this might change?
        pfd[0].fd = mosq_fd;
        pfd[0].events = POLLIN;
        if (mosquitto_want_write(mosq)){
            printf("Set POLLOUT\n");
            pfd[0].events |= POLLOUT;
        }
        if(poll(pfd, nfds, poll_timeout) < 0) {
            FATAL("Poll() failed with <%s>, exiting",strerror(errno));
            return EXIT_FAILURE;
        }
        // first checking the mosquitto socket
        if(pfd[0].revents & POLLOUT) {
            mosquitto_loop_write(mosq,1);
        }
        if(pfd[0].revents & POLLIN){
            int ret = mosquitto_loop_read(mosq, 1);
            if (ret == MOSQ_ERR_CONN_LOST ||
                ret == MOSQ_ERR_NO_CONN) {
                mqtt_connected = false;
            }
        }
        mosquitto_loop_misc(mosq);
    }


    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    INFO("bye");

    return EXIT_SUCCESS;
}
