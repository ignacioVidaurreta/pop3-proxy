#ifndef PC_2019B_01_ADMIN_REQUESTS_H
#define PC_2019B_01_ADMIN_REQUESTS_H

bool handle_user(char *user, int connSock);
bool handle_password(char * password, int conn_sock);
void print_commands();
void get_concurrent_connections(int conn_sock);
#endif /* PC_2019B_01_ADMIN_REQUESTS_H */