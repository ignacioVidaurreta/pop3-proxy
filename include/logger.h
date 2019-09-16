#ifndef LOGGER_H
#define LOGGER_H

#include <time.h>
#include <arpa/inet.h>


enum level {INFO, DEBUG, WARNING, ERROR, METRICS};

struct log_message {
    enum level msg_level;
    char *timestamp;
    char *message;
    int fd;
};


char *get_time();
int print_error(char *err_msg, char *timestamp);
void write_log(struct log_message* log);
void log_port(char* msg, in_port_t port);
void logger(enum level msg_level, char* message, char *timestamp);

#endif /* LOGGER_H */