#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>

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


int print_error(char* error_msg){
    logger(ERROR, error_msg);
    return 0;
}

void write_log(struct log_message* log){
    char *level = get_level_string(log->msg_level);
    if (level == NULL){
        perror("Error writing log");
        return;
    }
    fprintf(stdout, "%s\t%s\n", level, log->message);
}

void log_port(char *msg, in_port_t port_num ){
    fprintf(stdout, "[ INFO ]\t %s %d\n", msg, port_num);
}

/* 
 *  Handles the creation and writing of the log
 */
void logger(enum level msg_level, char *message){
    struct log_message* log = malloc(sizeof(*log));
    log->msg_level = msg_level;
    log->message = message;
    write_log(log);
}