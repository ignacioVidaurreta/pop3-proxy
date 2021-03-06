#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>

#include "include/pop3.h"
#include "include/buffer.h"
#include "include/server.h"
#include "include/metrics.h"

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

int read_from_server2(int server_fd, buffer *response, bool *error){
    int chars_read;
    if((chars_read=recv(server_fd, response, BUFFER_MAX_SIZE, 0))<0){
        perror("Error recieving data from server\n");
        *error = true;
    }

    return chars_read;
}

void write_to_server(int server_fd, char *cmd, struct state_manager* state){
    int n;
    if((n = send(server_fd, cmd, strlen(cmd), 0)) < 0){
        perror("Error sending data to server\n");
    }
    update_metrics_transfered_bytes(n);
    if(state->state != FILTER)
        state->state = RESPONSE;
}