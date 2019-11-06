#ifndef POP3__H
#define POP3__H

    #include "buffer.h"
    #include "pop3nio.h"
    extern struct config* options;
    
    int clean_up(int fd, int originFd, int failed);
    int serve_POP3_concurrent_blocking(const int server);
    void* handle_connection_pthread(void* args);
    void free_resources();

    #define TRUE 1
    #define FALSE 0

    // enum POP3_STATE {
    //     RESPONSE = 0,
    //     REQUEST = 1,
    //     FILTER = 2,
    //     END = 3,
    // };


    struct state_manager {
        enum pop3_state state;
        int is_single_line;
        int found_CR;
        int found_LF;
        int found_dot;
        int* main_to_external_fds;
        int* external_to_main_fds;
        int external_process;
    };

#endif
