#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "include/pop3.h"
#include "include/client.h"
#include "include/config.h"
#include "include/logger.h"
#include "include/buffer.h"
#include "include/metrics.h"


void write_response(int fd, char *response, struct state_manager* state){
    int n;
    if((n = send(fd, response, strlen(response), 0)) < 0){
        perror("Error sending data to client\n");
        return;
    }
    if (!strstr(response, "+OK") && state->is_single_line){
        logger(WARNING, "Origin returned error response. Awaiting next command from client.", get_time());
    }
    else {
        update_metrics_transfered_bytes(n);
    }       
}

void write_response_from_buffer(int fd, struct buffer_t *buff){
    int n;
    if((n = send(fd, buff->buffer, buff->curr_length, 0))<0){
        perror("Error sending data to client\n");
        return;
    }
    else {
        update_metrics_transfered_bytes(n);
    }
}
