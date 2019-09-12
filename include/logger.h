#ifndef LOGGER_H
#define LOGGER_H

#include <time.h>


enum level {INFO, DEBUG, WARNING, ERROR};

struct log_message {
    enum level msg_level;
    time_t timestamp;
    char *message;
    int fd;
};


int print_error(char *err_msg);
void write_log(struct log_message* log);
void log_port(char* msg, in_port_t port);
void logger(enum level msg_level, char* message);

#endif /* LOGGER_H */