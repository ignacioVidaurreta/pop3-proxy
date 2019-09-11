#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "include/parser.h"
/**
 *  Reads command from the filedescriptor and saves it to the command buffer
 * 
 * @param fd        proxy-client file descriptor
 * @param command   buffer where the read command will be stored.
 */
void read_command(int fd, char *command){
    memset(command, 0, strlen(command));
    if(recv(fd, command, 100, 0) < 0){
        perror("Error reading comand from filedescriptor\n");
        exit(1);
    }
}