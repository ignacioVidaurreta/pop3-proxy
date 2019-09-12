#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "include/server.h"

/*
 * Read output from server
 * 
 */
void read_from_server(int server_fd, char *response){
    memset(response, 0, strlen(response)); //Clear any previous response
    if(recv(server_fd, response, 100, 0)<0){
        perror("Error recieving data from server\n");
    }

}

void write_to_server(int server_fd, char *cmd){
    if(send(server_fd, cmd, 100, 0)<0){
        perror("Error sending data to server\n");
    }

}