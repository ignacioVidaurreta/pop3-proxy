#ifndef POP3__H
#define POP3__H

    #include "buffer.h"
    extern struct config* options;
    
    int clean_up(int fd, int originFd, int failed);
    int serve_POP3_concurrent_blocking(const int server);
    void* handle_connection_pthread(void* args);
    void free_resources();

    #define TRUE 1
    #define FALSE 0

    #define RESPONSE 2
    #define REQUEST 3
    #define END 4


    struct state_manager {
    int state;
    int is_single_line;
    int found_CR;
    int found_LF;
    int found_dot;
    };

#endif
