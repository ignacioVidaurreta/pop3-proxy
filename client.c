#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "include/pop3.h"
#include "include/client.h"
#include "include/config.h"
#include "include/logger.h"


void write_response(int fd, char *response, struct state_manager* state){
    if(send(fd, response, 100, 0)<0){
        perror("Error sending data to client\n");
        return;
    }
    if (!strstr(response, "+OK") && state->is_single_line){
        logger(WARNING, "Origin returned error response. Awaiting next command from client.", get_time());
    }
}