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
#include "include/config.h"

extern struct config* options;

int print_error(const char* error_msg){
    perror(error_msg);
    return 0;
}


int main(const int argc, char * const* argv){
    //Initialize configuration with default values
    initialize_config();
    update_config(argc, argv);
    
    const int server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server < 0) return print_error("Error: Unable to create socket");

    fprintf(stdout, "Listening on TCP port %ld\n", options->local_port);
    // man 7 ip. no importa reportar nada si falla.
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));
    if(bind(server, (struct sockaddr*)&options->proxy_address, sizeof(options->proxy_address)) < 0)
        return print_error("Error: Unable to bind socket");

    if(listen(server,20) < 0)
        return print_error("Error: Unable to listen");

    return serve_POP3_concurrent_blocking(server);
    
    

}