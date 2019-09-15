#include <unistd.h>
#include "include/transformations.h"

extern struct config* options;

int* create_process() {
    int pid = fork();
    if(pid == -1) {
        printf("%s failed to start external process. %s", TRANSFORMATION_START_ERR_MSG, EXIT_MSG);
        exit(1);
    }
    else if(pid == 0) {
        int* pipe_fds = malloc(2*sizeof(int));
        if(pipe(pipe_fds) == -1 || dup2(pipe_fds[0], STDOUT_FILENO) == -1 || dup2(pipe_fds[1], STDIN_FILENO) == -1) {
            printf("%s failed to create pipes. %s", TRANSFORMATION_START_ERR_MSG, EXIT_MSG);
            exit(1);
        }

        return execl("/bin/sh", "sh", "-c", options->cmd, NULL);
    }

    return pid;
}
