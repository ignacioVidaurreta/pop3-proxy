#ifndef CLIENT_H
#define CLIENT_H
#include "buffer.h"

void write_response(int fd, char *response);
void write_response_from_buffer(int fd, struct buffer_t *buff);

#endif /* CLIENT_H */