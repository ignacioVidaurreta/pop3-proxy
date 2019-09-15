#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <strings.h>
#include "include/pop3.h"
#include "include/parser.h"

/**
 *  Reads command from the filedescriptor and saves it to the command buffer
 * 
 * @param fd        proxy-client file descriptor
 * @param command   buffer where the read command will be stored.
 */
void read_command(int fd, char *command){
    memset(command, 0, strlen(command));
    if((recv(fd, command, 100, 0)) < 0){
        perror("Error reading comand from filedescriptor\n");
        exit(1);
    }
}


void read_multiline_command(char* buffer, struct state_manager* state){
    int ended = FALSE;

    if(buffer[0] == '-'){
        state->state = REQUEST;
        return; //Do not parse, it's actually single line
    }

    for(int i = 0; i < BUFFER_MAX_SIZE && !ended; i++){
        char c = buffer[i];
        switch(c){
            case '\r':
                state->found_CR = TRUE;
                break;
            case '\n':
                if(state->found_CR){
                    if(state->found_dot){
                        ended = TRUE;
                    }
                    state->found_LF = TRUE;
                }
                break;
            case '.':
                if(state->found_CR && state->found_LF){
                    state->found_dot = TRUE;
                }else{
                    state->found_dot = FALSE; //Take into account the \r\n..\r\n case
                }
                state->found_CR  = FALSE;
                state->found_LF  = FALSE;
                break;
            default: //It isn't any interesting character, so restore everything to default
                state->found_CR  = FALSE;
                state->found_LF  = FALSE;
                state->found_dot = FALSE;
                break;
        }

        if(ended){
            state->state = REQUEST;
        }
    }

}

void parse_response(char* buffer, struct state_manager* state) {
    if(state->is_single_line){
        if(strcmp(buffer, "+OK Logging out\r\n") == 0) {
            state->state = END;
        }
        else{
            state->state = REQUEST;
        }
    }else{
        read_multiline_command(buffer, state);
    }

}
void parse_command(char* buffer, struct state_manager* state) {
    char cmd[8], extra[64];
    sscanf(buffer,"%s %s", cmd, extra);

    if (strcasecmp(cmd, "RETR") == 0 || strcasecmp(cmd, "LIST") == 0 || strcasecmp(cmd, "CAPA") == 0 || strcasecmp(cmd, "UIDL") == 0 || strcasecmp(cmd, "TOP") == 0) {
        state->is_single_line = FALSE;
    }
    else
        state->is_single_line = TRUE;
}