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
#include "include/logger.h"
#include "include/metrics.h"


extern struct config* options;
struct metrics_manager * metrics;

int main(const int argc, char * const* argv){
    //Initialize configuration with default values
    initialize_config();
    init_metrics_manager();

    update_config(argc, argv);
    
    const int server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server < 0) return print_error("Unable to create socket", get_time());

    log_port("Listening on TCP port", options->local_port);
    // man 7 ip. no importa reportar nada si falla.
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));
    if(bind(server, (struct sockaddr*)&options->proxy_address, sizeof(options->proxy_address)) < 0)
        return print_error("Unable to bind socket", get_time());

    if(listen(server,20) < 0)
        return print_error("Unable to listen", get_time());

    return serve_POP3_concurrent_blocking(server);
}