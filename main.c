#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>

#include <unistd.h>
#include <sys/types.h>   // socket
#include <sys/socket.h>  // socket
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "include/pop3.h"
#include "include/config.h"
#include "include/logger.h"
#include "include/metrics.h"
#include "include/pop3nio.h"
#include "include/management.h"
#include "include/selector.h"

static bool done = false;
extern struct config* options;
struct metrics_manager * metrics;

static void
sigterm_handler(const int signal) {
    printf("signal %d, cleaning up and exiting\n",signal);
    done = true;
    free_resources();
    exit(0);
}

int main(const int argc, char* const* argv) {
     //Initialize configuration with default values
    initialize_config();
    init_metrics_manager();

    update_config(argc, argv);

    // no tenemos nada que leer de stdin
    close(0);

    const char       *err_msg = NULL;
    selector_status   ss      = SELECTOR_SUCCESS;
    fd_selector selector      = NULL;

    if (((struct sockaddr*)&options->proxy_address)->sa_family == AF_INET)
        ((struct sockaddr_in*)&options->proxy_address)->sin_port = htons(options->local_port);
    else if (((struct sockaddr*)&options->proxy_address)->sa_family == AF_INET6)
        ((struct sockaddr_in6*)&options->proxy_address)->sin6_port = htons(options->local_port);

    if (((struct sockaddr*)&options->management_address)->sa_family == AF_INET)
        ((struct sockaddr_in*)&options->management_address)->sin_port = htons(options->management_port);
    else if (((struct sockaddr*)&options->management_address)->sa_family == AF_INET6)
        ((struct sockaddr_in6*)&options->management_address)->sin6_port = htons(options->management_port);


    const int server = socket(((struct sockaddr*)&options->proxy_address)->sa_family,
            SOCK_STREAM, IPPROTO_TCP);
    const int management_server = socket(((struct sockaddr*)&options->management_address)->sa_family, 
            SOCK_STREAM, IPPROTO_SCTP);

    if(server < 0) {
        err_msg = "Unable to create server socket :(";
        goto finally;
    }

    if (management_server < 0){
        err_msg = "Unable to create management socket :(";
        goto finally;
    }

    // man 7 ip. no importa reportar nada si falla.
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));
    setsockopt(management_server, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

    if(bind(server, (struct sockaddr*)&options->proxy_address, sizeof(options->proxy_address)) < 0){
        err_msg = "Unable to bind server socket :(";
        goto finally;
    }

    if(bind(management_server, (struct sockaddr*)&(options->management_address), sizeof(options->management_address)) < 0){
        err_msg = "Unable to bind management socket :(";
        goto finally;
    }

    if(listen(server,20) < 0){
        err_msg = "Unable to listen on server :(";
        goto finally;
    }

    if (listen(management_server, 20)){
        err_msg = "Unable to listen on management server :(";
        goto finally;
    }

    log_port("Listening on TCP port", options->local_port);

    // Registrar sigterm es útil para terminar el programa normalmente.
    // Esto ayuda mucho en herramientas como valgrind.
    signal(SIGTERM, sigterm_handler);
    signal(SIGINT,  sigterm_handler);

    if(selector_fd_set_nio(server) == -1) {
        err_msg = "Getting server socket flags";
        goto finally;
    }

    if(selector_fd_set_nio(management_server) == -1) {
        err_msg = "Getting management server socket flags";
        goto finally;
    }

    const struct selector_init conf = {
        .signal = SIGALRM,
        .select_timeout = {
            .tv_sec  = 10,
            .tv_nsec = 0,
        },
    };

    if(0 != selector_init(&conf)) {
        err_msg = "initializing selector";
        goto finally;
    }

    selector = selector_new(1024);
    if(selector == NULL) {
        err_msg = "unable to create selector";
        goto finally;
    }
    const struct fd_handler pop3filter = {
        .handle_read       = pop3filter_passive_accept, 
        .handle_write      = NULL,
        .handle_close      = NULL, // nada que liberar
    };

    const struct fd_handler management = {
        .handle_read       = management_passive_accept,
        .handle_write      = NULL,
        .handle_close      = NULL, // nada que liberar
    };
    

    ss = selector_register(selector, server, &pop3filter, OP_READ, NULL);
    ss = selector_register(selector, management_server, &management, OP_READ, NULL);

    if(ss != SELECTOR_SUCCESS) {
        err_msg = "Registering fd";
        goto finally;
    }

    while(!done) {
        err_msg = NULL;
        ss = selector_select(selector);
        if(ss != SELECTOR_SUCCESS) {
            err_msg = "serving";
            goto finally;
        }
    }

    if(err_msg == NULL) {
        err_msg = "closing";
    }

    int ret = 0;

finally:

    if(ss != SELECTOR_SUCCESS) {
        fprintf(stderr, "%s: %s\n", (err_msg == NULL) ? "": err_msg,
                                  ss == SELECTOR_IO
                                      ? strerror(errno)
                                      : selector_error(ss));
        ret = 2;
    } else if(err_msg) {
        perror(err_msg);
        ret = 1;
    }
    if(selector != NULL) {
        selector_destroy(selector);
    }

    selector_close();
    free_resources();
    // TODO(@team): pop3filter_pool_destroy();

    if(server >= 0) {
        close(server);
    }
    return ret;
}