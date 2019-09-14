#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <strings.h>
#include "include/parser.h"
#include "include/pop3.h"
/**
 *  Reads command from the filedescriptor and saves it to the command buffer
 * 
 * @param fd        proxy-client file descriptor
 * @param command   buffer where the read command will be stored.
 */
void read_command(int fd, char *command, int * is_single_line){
    int n;
    memset(command, 0, strlen(command));
    if((n = recv(fd, command, 100, 0)) < 0){
        perror("Error reading comand from filedescriptor\n");
        exit(1);
    }

    char cmd[8], extra[64];
    sscanf(command,"%s %s", cmd, extra);

    if ( strcasecmp(cmd, "RETR") == 0 || strcasecmp(cmd, "LIST") == 0 || strcasecmp(cmd, "CAPA") == 0 || strcasecmp(cmd, "UIDL") == 0 || strcasecmp(cmd, "TOP") == 0) {
        *is_single_line = FALSE;
    }
    else
        *is_single_line = TRUE;

    fprintf(stdout,"%d \n",*is_single_line);
}