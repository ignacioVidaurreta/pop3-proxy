#ifndef POP3__H
#define POP3__H

    extern struct config* options;
    
    int clean_up(int fd, int originFd, int failed);
    int serve_POP3_concurrent_blocking(const int server);
    void* handle_connection_pthread(void* args);
    void free_resources();
    void print_metrics();

    #define TRUE 1
    #define FALSE 0

    #define RESPONSE 2
    #define REQUEST 3
    #define END 4

    #define BUFFER_MAX_SIZE 512

    struct state_manager {
    int state;
    int is_single_line;
    int found_CR;
    int found_LF;
    int found_dot;
    };

    struct metrics_manager {
    int concurrent_connections;
    int number_of_connections;
    int transfered_bytes;
    };

#endif
