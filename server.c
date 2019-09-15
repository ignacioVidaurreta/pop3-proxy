#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "include/pop3.h"
#include "include/buffer.h"
#include "include/server.h"

extern struct state_manager* state;


/*
 * Read output from server
 * 
 */
int read_from_server(int server_fd, char *response){
    int chars_read;
    if((chars_read=recv(server_fd, response, BUFFER_MAX_SIZE, 0))<0){
        perror("Error recieving data from server\n");
    }

    return chars_read;
}

void write_to_server(int server_fd, char *cmd, struct state_manager* state){
    if(send(server_fd, cmd, strlen(cmd), 0)<0){
        perror("Error sending data to server\n");
    }
    state->state = RESPONSE;

}