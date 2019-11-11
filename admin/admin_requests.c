#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>

#define STREAM 0
#define MAX_DATAGRAM_SIZE 255 + 3

void print_error_msg(uint8_t error_code){
    switch(error_code){
        case 0x01:
            printf("Authentication Error \n");
            break;
        case 0x02:
            printf("Invalid command \n");
            break;
        case 0x03:
            printf("Invalid arguments \n");
            break;
        case 0x04:
        default:
            printf("General Error \n");
    }
}
bool handle_user(char *user, int connSock) {
    uint8_t command = 0x00, nargs = 1;
    int ret;
    uint8_t datagram[MAX_DATAGRAM_SIZE];
    uint8_t response[MAX_DATAGRAM_SIZE];

    datagram[0] = command;
    datagram[1] = nargs;
    size_t length = strlen(user);
    datagram[2] = length;
    memcpy(datagram + 3, user, length);

    ret = sctp_sendmsg(connSock, (const void *) datagram, 3 + length,
                       NULL, 0, 0, 0, STREAM, 0, 0);

    ret = sctp_recvmsg(connSock, (void *) response, MAX_DATAGRAM_SIZE,
                       (struct sockaddr *) NULL, 0, 0, 0);
    if (response[0] != 0)  {
        return false;
    }
    return true;
}

bool handle_password(char * password, int conn_sock){
    uint8_t command = 0x01, nargs = 1;
    int ret;
    uint8_t datagram[MAX_DATAGRAM_SIZE];
    uint8_t response[MAX_DATAGRAM_SIZE];

    datagram[0] = command;
    datagram[1] = nargs;
    size_t length = strlen(password);
    datagram[2] = length;
    memcpy(datagram + 3, password, length);
    ret = sctp_sendmsg(conn_sock, (const void *) datagram, 3 + length,
                       NULL, 0, 0, 0, STREAM, 0, 0);
    ret = sctp_recvmsg(conn_sock, (void *) response, MAX_DATAGRAM_SIZE,
                       (struct sockaddr *) NULL, 0, 0, 0);
    
    return response[0] == 0;
}

void print_commands(){
    printf("Are you lost? Do you need a hand?\n");
    printf("---------------------------------------------------------------\n");
    printf("> 'help' To display this very useful message\n");
    printf("> 'quit' To exit the manager\n");
    printf("> 'connections' To get the number of concurrent connections\n");
    printf("> 'get_transformation'[WIP] Get current active transformation\n");
    printf("> 'set_transformation'[WIP] Change current active transformation\n");
    printf("> 'bytes' To get number of transferred bytes\n");
}

void send_msg_no_arguments(int conn_sock, char* fmt, uint8_t command, uint8_t nargs){
    int ret;
    uint8_t datagram[MAX_DATAGRAM_SIZE];
    uint8_t response[MAX_DATAGRAM_SIZE];
    datagram[0] = command;
    datagram[1] = nargs;
    ret = sctp_sendmsg(conn_sock, (const void *) datagram, 2,
                       NULL, 0, 0, 0, STREAM, 0, 0);
    ret = sctp_recvmsg(conn_sock, (void *) response, MAX_DATAGRAM_SIZE,
                       (struct sockaddr *) NULL, 0, 0, 0);
    if (response[0] == 0) {
        char data[response[1] + 1];
        memcpy(data, response + 2, response[1]);
        data[response[1]] = '\0';
        printf(fmt, data);
    } else {
        print_error_msg(response[0]);
    }
}

void get_active_transformation(int conn_sock){
    char* format_msg = "Current transformation: %s \n";
    uint8_t command = 0x04, nargs = 0;
    send_msg_no_arguments(conn_sock, format_msg, command, nargs);
}

void get_concurrent_connections(int conn_sock){
    char* format_msg = "Concurrent connections: %s \n";
    uint8_t command = 0x02, nargs = 0;
    send_msg_no_arguments(conn_sock, format_msg, command, nargs);
}

void get_transferred_bytes(int conn_sock){
    char* format_msg = "Bytes transferred: %s \n";
    uint8_t command = 0x05, nargs = 0;
    send_msg_no_arguments(conn_sock, format_msg, command, nargs);
}