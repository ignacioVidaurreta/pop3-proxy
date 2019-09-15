#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <time.h>

#include "include/logger.h"

char *get_level_string(enum level msg_level){
    switch(msg_level){
        case INFO:
            return "[ INFO ]";
        case WARNING:
            return "[ WARNING ]";
        case ERROR:
            return "[ ERROR ]";
        case DEBUG:
            return "[ DEBUG ]";
        default:
            return NULL;
    }
}
/*
 * Code taken from: https://stackoverflow.com/questions/1442116/how-to-get-the-date-and-time-values-in-a-c-program
 */
char *get_time(){
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char *time = malloc(10*sizeof(char)); //TODO: free in right place
    sprintf(time, "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
    return time;
}


int print_error(char* error_msg, char *timestamp){
    logger(ERROR, error_msg, timestamp);
    return 0;
}

void write_log(struct log_message* log){
    char *level = get_level_string(log->msg_level);
    if (level == NULL){
        perror("Error writing log");
        return;
    }
    fprintf(stdout, "[%s] %s\t%s\n", log->timestamp,level, log->message);
}

void log_port(char *msg, in_port_t port_num ){
    char* current_time = get_time();
    fprintf(stdout, "[%s][ INFO ]\t%s %d\n", current_time, msg, port_num);
}

/* 
 *  Handles the creation and writing of the log
 */
void logger(enum level msg_level, char *message, char *timestamp){
    struct log_message* log = malloc(sizeof(*log)); //TODO: free in right place
    log->msg_level = msg_level;
    log->message   = message;
    log->timestamp = timestamp;
    write_log(log);
}