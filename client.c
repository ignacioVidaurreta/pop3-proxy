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


void write_response(int fd, uint8_t* response, struct state_manager* state){
    int n;
    if((n = send(fd, response, strlen((char*)response), 0)) < 0){
        perror("Error sending data to client\n");
        return;
    }
    if (!strstr((char*)response, "+OK") && state->is_single_line){
        logger(WARNING, "Origin returned error response. Awaiting next command from client.", get_time());
    }
    else {
        update_metrics_transfered_bytes(n);
    }       
}

// void write_response_from_buffer(int fd, buffer *buff){
//     int n;
//     if((n = send(fd, buff->buffer, buff->curr_length, 0))<0){
//         perror("Error sending data to client\n");
//         return;
//     }
//     else {
//         update_metrics_transfered_bytes(n);
//     }
// }
