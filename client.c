#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "include/client.h"

void write_response(int fd, char *response){
    if(send(fd, response, 100, 0)<0){
        perror("Error sending data to client\n");
    }
}