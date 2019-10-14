#include <stdio.h>
#include <stdlib.h>

#include "include/buffer.h"

struct buffer_t * init_buffer(){
    struct buffer_t *buff;
    buff                  = malloc(sizeof(*buff));
    buff -> write_pointer = 0;
    buff -> buffer        = malloc(sizeof(char) * BUFFER_MAX_SIZE);
    buff -> curr_length   = BUFFER_MAX_SIZE;
    
    return buff;
}


void expand_buffer(struct buffer_t *buff){
    buff->curr_length += BUFFER_MAX_SIZE;
    buff->buffer = realloc(buff->buffer,sizeof(char) * buff->curr_length);
}

// void reset_buffer(struct buffer_t *buff){
//     free(buff->buffer)

// }

void free_buffer(struct buffer_t *buff){
    free(buff->buffer);
    free(buff);
}

// -------------------------------------------------------------------------------------------------------------------------

