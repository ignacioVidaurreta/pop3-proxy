#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#include "include/transformations.h"
#include "include/config.h"
#include "include/pop3.h"
#include "include/logger.h"

extern struct config* options;

int* create_process() {
    int pid = fork();
    int* pipe_fds = malloc(2*sizeof(int));
    if(pid == -1) {
        printf("%s failed to start external process. %s", TRANSFORMATION_START_ERR_MSG, EXIT_MSG);
        exit(1);
    }
    else if(pid == 0) {
        if(pipe(pipe_fds) == -1 || dup2(pipe_fds[0], STDOUT_FILENO) == -1 || dup2(pipe_fds[1], STDIN_FILENO) == -1) {
            printf("%s failed to create pipes. %s", TRANSFORMATION_START_ERR_MSG, EXIT_MSG);
            exit(1);
        }

        //return execl("/bin/sh", "sh", "-c", options->cmd, NULL);
        logger(INFO, "Running transformation on email", get_time());
        execl("/bin/sh", "sh", "-c", options->cmd, NULL);
    }else{
        waitpid(pid, NULL, 0);
    }

    

    return pipe_fds;
}

void write_buffer(char* buffer, int write_fd){
    if(send(write_fd, buffer, strlen(buffer), 0) < 0){
        print_error("Error writing to fd", get_time());
    }

    close(write_fd);
}

void read_transformation(char* buffer, int read_fd){
    if(recv(read_fd, buffer, BUFFER_MAX_SIZE, 0) < 0){
        print_error("Error reading from fd", get_time());
    }
    close(read_fd);
}
