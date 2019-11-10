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

bool handle_user(char *user, int connSock) {
    uint8_t command = 0, nargs = 1;
    int ret;
    uint8_t datagram[MAX_DATAGRAM_SIZE];
    uint8_t res[MAX_DATAGRAM_SIZE];

    datagram[0] = command;
    datagram[1] = nargs;
    size_t length = strlen(user);
    datagram[2] = length;
    memcpy(datagram + 3, user, length);

    ret = sctp_sendmsg(connSock, (const void *) datagram, 3 + length,
                       NULL, 0, 0, 0, STREAM, 0, 0);

    ret = sctp_recvmsg(connSock, (void *) res, MAX_DATAGRAM_SIZE,
                       (struct sockaddr *) NULL, 0, 0, 0);
    if (res[0] != 0)  {
        return false;
    }
    return true;
}

bool handle_password(char * password, int conn_sock){
    uint8_t command = 1, nargs = 1;
    int ret;
    uint8_t datagram[MAX_DATAGRAM_SIZE];
    uint8_t res[MAX_DATAGRAM_SIZE];

    datagram[0] = command;
    datagram[1] = nargs;
    size_t length = strlen(password);
    datagram[2] = length;
    memcpy(datagram + 3, password, length);
    ret = sctp_sendmsg(conn_sock, (const void *) datagram, 3 + length,
                       NULL, 0, 0, 0, STREAM, 0, 0);
    ret = sctp_recvmsg(conn_sock, (void *) res, MAX_DATAGRAM_SIZE,
                       (struct sockaddr *) NULL, 0, 0, 0);
    
    return res[0] == 0;
}