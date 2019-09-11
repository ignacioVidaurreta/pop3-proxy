#ifndef SERVER_H
#define SERVER_H

void read_from_server(int fd, char *response);

void write_to_server(int server_fd, char *cmd);

#endif /*SERVER_H*/