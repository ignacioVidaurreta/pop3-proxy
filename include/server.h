#ifndef SERVER_H
#define SERVER_H

#include "pop3.h"

int read_from_server(int fd, char *response);

void write_to_server(int server_fd, char *cmd, struct state_manager* state);
int read_from_server2(int server_fd, buffer *response, bool *error);

#endif /*SERVER_H*/