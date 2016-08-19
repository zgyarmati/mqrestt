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

#include <curl/curl.h>


#include "configuration.h"
#include "logging.h"
//#include <config.h>
#include <mosquitto.h>



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
            lockf(pid_fd, F_ULOCK, 0);
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
    chdir("/");

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
        write(pid_fd, str, strlen(str));
    }
}

/**
 * \brief Print help for this application
 */
void print_help(void)
{
    printf("\nUsage: %s [OPTIONS]\n\n", app_name);
    printf("Options:\n");
    printf("   -h --help                 Print this help\n");
    printf("   -c --configfile filename  Read configuration from the file\n");
    printf("   -d --daemon               Daemonize this application\n");
    printf("\n");
}


int
rest_post(Configuration *conf, const char *url, const char *payload )
{
    CURL *curl;
    CURLcode res;
    int retval = 0;
    const int len = strlen(config->webservice_baseurl);
    DEBUG("len url: %d", strlen(url));
    #define URL_MAX_SIZE 256
    char full_url[URL_MAX_SIZE];
    strncpy(full_url,config->webservice_baseurl, URL_MAX_SIZE);
    if(full_url[len-1] != '/'){
        DEBUG("Need to append a '/'");
        full_url[len] = '/';
        full_url[len + 1] = '\0';
    }
    strncat(full_url,url,URL_MAX_SIZE-len-1);
    INFO("FULL URL: %s", full_url);

    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, full_url);
        if (payload == NULL){
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
        }
        else {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
        }
        res = curl_easy_perform(curl);
        if(res != CURLE_OK){
            ERROR("curl_easy_perform() failed: %s", curl_easy_strerror(res));
            retval = -1;
        }
        curl_easy_cleanup(curl);
    }
    return retval;
}


void
mqtt_cb_msg(struct mosquitto *mosq, void *userdata,
                  const struct mosquitto_message *msg)
{
    char **topics;
    int topic_count;
    //tailoring the url
    char *url = msg->topic + strlen(config->mqtt_topic) + 1; // +1 the '/'

    mosquitto_sub_topic_tokens_free(&topics, topic_count);
    DEBUG("Received msg on topic: %s\n", msg->topic);
    if(msg->payload != NULL){
        DEBUG("Payload: %s", msg->payload);
    }
    rest_post(config, url, msg->payload);
}


void
mqtt_cb_connect(struct mosquitto *mosq, void *userdata, int result)
{
    int i;
    if(!result){
        mosquitto_subscribe(mosq, NULL, "facility/#", 2);
    }else{
        WARNING("MQTT Connect failed\n");
    }
}

void
mqtt_cb_subscribe(struct mosquitto *mosq, void *userdata, int mid,
                        int qos_count, const int *granted_qos)
{
    int i;

    INFO("Subscribed (mid: %d): %d", mid, granted_qos[0]);
    for(i=1; i<qos_count; i++){
        INFO("\t %d", granted_qos[i]);
    }
}

void
mqtt_cb_disconnect(struct mosquitto *mosq, void *userdat, int rc)
{
    WARNING("MQTT disconnect, error: %d: %s",rc, mosquitto_strerror(rc));
}

//MOSQ_LOG_INFO MOSQ_LOG_NOTICE MOSQ_LOG_WARNING MOSQ_LOG_ERR MOSQ_LOG_DEBUG
void
mqtt_cb_log(struct mosquitto *mosq, void *userdata,
                  int level, const char *str)
{
    switch(level){
        case MOSQ_LOG_DEBUG:
            DEBUG(str);
            break;
        case MOSQ_LOG_INFO:
        case MOSQ_LOG_NOTICE:
            INFO(str);
            break;
        case MOSQ_LOG_WARNING:
            WARNING(str);
            break;
        case MOSQ_LOG_ERR:
            ERROR(str);
            break;
        default:
            FATAL("Unknown MOSQ loglevel!");
    }
}

int main(int argc, char *argv[])
{
    static struct option long_options[] = {
        {"configfile", required_argument, 0, 'c'},
        {"help", no_argument, 0, 'h'},
        {"daemon", no_argument, 0, 'd'},
        {NULL, 0, 0, 0}
    };
    int value, option_index = 0, ret;
    int start_daemonized = 0;

    app_name = argv[0];

    /* Try to process all command line arguments */
    while( (value = getopt_long(argc, argv, "c:dh", long_options, &option_index)) != -1) {
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
    ret = log_init(config->loglevel, config->logtarget, config->logfile,
              config->logfacility, 0);
    INFO("bordeaux-eventdispatcher started, pid: %d", getpid());
    INFO("bordeaux-eventdispatcher version: %s", "ver");
    INFO("libmosquitto version: %d", mosquitto_lib_version(NULL, NULL, NULL));
//    rest_post(config,"3213213/online", "");

    int keepalive = 60;
    bool clean_session = true;
    struct mosquitto *mosq = NULL;

    mosquitto_lib_init();
    mosq = mosquitto_new(NULL, clean_session, NULL);
    if(!mosq){
        fprintf(stderr, "Error: Out of memory.\n");
        return 1;
    }
    mosquitto_log_callback_set(mosq, mqtt_cb_log);
    mosquitto_connect_callback_set(mosq, mqtt_cb_connect);
    mosquitto_message_callback_set(mosq, mqtt_cb_msg);
    mosquitto_subscribe_callback_set(mosq, mqtt_cb_subscribe);
    mosquitto_disconnect_callback_set(mosq, mqtt_cb_disconnect);

    running = 1; // changed from signal handler
    signal(SIGINT, handle_signal);
    while(running){ //we try until we succeed, or we killed
        if(mosquitto_connect(mosq, config->mqtt_broker_host,
                                config->mqtt_broker_port, keepalive)){
            ERROR("Unable to connect, host: %s, port: %d\n",
                    config->mqtt_broker_host, config->mqtt_broker_port);
            sleep(2);
            continue;
        }
        break;
    }

    /* Reset signal handling to default behavior */
    signal(SIGINT, SIG_DFL);
    if (running){
        mosquitto_loop_forever(mosq, -1, 1);
    }

    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    INFO("bye");

    return EXIT_SUCCESS;
}
