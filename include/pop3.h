#ifndef POP3__H
#define POP3__H

    extern struct config* options;
    
    int clean_up(int fd, int originFd, int failed);
    int serve_POP3_concurrent_blocking(const int server);
    void* handle_connection_pthread(void* args);

    #define TRUE 1
    #define FALSE 0

#endif
