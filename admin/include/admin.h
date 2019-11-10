#ifndef PC_2019B_01_ADMIN_H
#define PC_2019B_01_ADMIN_H
#include <stdint.h>

bool resolve_address(char *address, uint16_t port, struct addrinfo ** addrinfo);
static uint16_t parse_port(const char * port);
void print_usage();
bool is_valid_username(char* username);
bool is_valid_password(char* pass);


#endif /* PC_2019B_01_ADMIN_H */