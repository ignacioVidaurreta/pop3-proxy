//TODO: check POSIX version #define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/types.h>
#include <signal.h>

#include "include/transformations.h"
#include "include/config.h"
#include "include/pop3.h"
#include "include/logger.h"

extern struct config* options;

void create_process(struct state_manager* state) {
    if(state->external_process)
        return;
    state->main_to_external_fds = malloc(2*sizeof(int));
    state->external_to_main_fds = malloc(2*sizeof(int));
    if(pipe(state->main_to_external_fds) == -1 || pipe(state->external_to_main_fds) == -1) {
        printf("%s failed to create pipes. %s", TRANSFORMATION_START_ERR_MSG, EXIT_MSG);
        exit(1);
    }
    state->external_process = fork();
    if(state->external_process == -1) {
        printf("%s failed to start external process. %s", TRANSFORMATION_START_ERR_MSG, EXIT_MSG);
        exit(1);
    }
    else if(state->external_process == 0) {
        logger(INFO, "Running transformation on email", get_time());
        if(dup2(state->main_to_external_fds[0], STDIN_FILENO) == -1 || dup2(state->external_to_main_fds[1], STDOUT_FILENO) == -1) {
            printf("%s failed to create pipes. %s", TRANSFORMATION_START_ERR_MSG, EXIT_MSG);
            exit(1);
        }
        execl("/bin/sh", "sh", "-c", options->cmd, NULL);
    }
}

void write_buffer(char* buffer, int write_fd){
    if(dprintf(write_fd, buffer) < 0)
        print_error("Error writing to fd", get_time());
}

void read_transformation(char* buffer, int read_fd) {
    if(read(read_fd, buffer, strlen(buffer)) < 0)
        print_error("Error reading from fd", get_time());
}

void update_external_process_data(struct state_manager* state) {
    if(state->state == FILTER)
        return;
    kill(state->external_process, SIGKILL);
    close(state->external_to_main_fds[0]);
    close(state->external_to_main_fds[1]);
    close(state->main_to_external_fds[0]);
    close(state->main_to_external_fds[1]);
    free(state->external_to_main_fds);
    free(state->main_to_external_fds);
    state->external_process = 0;
}

