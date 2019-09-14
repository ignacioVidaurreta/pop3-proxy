#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <strings.h>
#include "include/parser.h"
#include "include/pop3.h"

#define BUFFER_MAX_SIZE 512
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


void read_multiline_command(char buffer[]){
    int found_CR = FALSE;
    int found_LF = FALSE;
    int found_dot = FALSE;
    int ended = FALSE;

    if(buffer[0] == '-'){
        return; //Do not parse, it's actually single line
    }

    for(int i = 0; i < BUFFER_MAX_SIZE && !ended; i++){
        char c = buffer[i];
        switch(c){
            case '\r':
                found_CR = TRUE;
                break;
            case '\n':
                if(found_CR){
                    if(found_dot){
                        ended = TRUE;
                    }
                    found_LF = TRUE;
                }
            case '.':
                if(found_CR && found_LF){
                    found_dot = TRUE;
                }else{
                    found_dot = FALSE; //Take into account the \r\n..\r\n case
                }
                found_CR  = FALSE;
                found_LF  = FALSE;
            default: //It isn't any interesting character, so restore everything to default
                found_CR  = FALSE;
                found_LF  = FALSE;
                found_dot = FALSE;
        }

        if(ended){
            //STATUS = REQUEST
        }
    }

}