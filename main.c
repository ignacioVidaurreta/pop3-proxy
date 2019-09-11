#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <pthread.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>

#include "include/pop3.h"

int print_error(const char* error_msg){
    perror(error_msg);
    return 0;
}


int main(const int argc, const char **argv){
    unsigned port = 1100;

    //TODO(Nachito) Change port via command-line
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr)); // Fill struct with zeros
    addr.sin_family         = AF_INET;
    addr.sin_addr.s_addr    = htonl(INADDR_ANY);
    addr.sin_port           = htons(port);

    const int server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server < 0) return print_error("Error: Unable to create socket");

    fprintf(stdout, "Listening on TCP port %d\n", port);
    // man 7 ip. no importa reportar nada si falla.
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));
    if(bind(server, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        return print_error("Error: Unable to bind socket");

    if(listen(server,20) < 0)
        return print_error("Error: Unable to listen");

    return servePOP3ConcurrentBlocking(server);
    
    

}