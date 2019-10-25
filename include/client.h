#ifndef CLIENT_H
#define CLIENT_H

void write_response(int fd, char *response, struct state_manager* state);
//void write_response_from_buffer(int fd, struct buffer_t *buff);

#endif /* CLIENT_H */