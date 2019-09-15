#ifndef SERVER_H
#define SERVER_H

int read_from_server(int fd, char *response);

void write_to_server(int server_fd, char *cmd, struct state_manager* state);

#endif /*SERVER_H*/