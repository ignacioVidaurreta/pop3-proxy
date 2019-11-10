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
        case FAILURE:
            return "[ ERROR ]";
        case DEBUG:
            return "[ DEBUG ]";
        case METRICS:
            return "[ METRICS ]";
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
    char* time= malloc(10*sizeof(char));
    sprintf(time, "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
    return time;
}


int print_error(char* error_msg, char *timestamp){
    logger(FAILURE, error_msg, timestamp);
    return 0;
}

void write_log(struct log_message* log){
    char *level = get_level_string(log->msg_level);
    if (level == NULL){
        perror("Error writing log");
        return;
    }
    char* color_modifier;
    if(log->msg_level == FAILURE){
        color_modifier = "\033[0;31m"; //RED
    }else if(log->msg_level == WARNING){
        color_modifier = "\033[0;33m"; //YELLOW
    }else{
        color_modifier = "\033[0;36m"; //CYAN
    }
    fprintf(stdout, "%s[%s]%s\033[0m\t%s\n", color_modifier, log->timestamp,level, log->message);
}

void log_port(char *msg, in_port_t port_num ){
    char* current_time = get_time();
    fprintf(stdout, "\033[0;36m[%s][ INFO ]\033[0m\t%s %d\n", current_time, msg, port_num);
    free(current_time);
}

/* 
 *  Handles the creation and writing of the log
 */
void logger(enum level msg_level, char *message, char *timestamp){
    struct log_message* log = malloc(sizeof(*log));
    log->msg_level = msg_level;
    log->message   = message;
    log->timestamp = timestamp;
    write_log(log);
    free(log->timestamp);
    free(log);
}
