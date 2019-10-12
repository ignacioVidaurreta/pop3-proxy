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

#include "include/selector.h"

static bool done = false;
extern struct config* options;
struct metrics_manager * metrics;

static void
sigterm_handler(const int signal) {
    printf("signal %d, cleaning up and exiting\n",signal);
    done = true;
}

int
main(const int argc, const char **argv) {
     //Initialize configuration with default values
    initialize_config();
    init_metrics_manager();

    update_config(argc, argv);

    // no tenemos nada que leer de stdin
    close(0);

    const char       *err_msg = NULL;
    selector_status   ss      = SELECTOR_SUCCESS;
    fd_selector selector      = NULL;

    //TODO: AF_INET puede ser IPv4 o IPv6

    const int server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(server < 0) {
        err_msg = "unable to create socket";
        goto finally;
    }

    // man 7 ip. no importa reportar nada si falla.
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

    if(bind(server, (struct sockaddr*)&options->proxy_address, sizeof(options->proxy_address)) < 0){
        err_msg = "unable to bind socket";
        goto finally;
    }

/*    
    NO SE QUE ES LEL

    struct sctp_initmsg initmsg;
  	memset(&initmsg, 0, sizeof(initmsg));
 	initmsg.sinit_num_ostreams = 1;
  	initmsg.sinit_max_instreams = 1;
  	initmsg.sinit_max_attempts = 4;
  	setsockopt(managementServer, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof(initmsg)); */

    if(listen(server,20) < 0){
        err_msg = "unable to listen";
        goto finally;
    }

    log_port("Listening on TCP port", options->local_port);
    fprintf(stdout, "Listening on TCP port %d\n", options->local_port);

    // registrar sigterm es útil para terminar el programa normalmente.
    // esto ayuda mucho en herramientas como valgrind.
    signal(SIGTERM, sigterm_handler);
    signal(SIGINT,  sigterm_handler);

    if(selector_fd_set_nio(server) == -1) {
        err_msg = "getting server socket flags";
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
        .handle_read       = pop3filter_passive_accept, //TODO: necesitamos esto en el nio
        .handle_write      = NULL,
        .handle_close      = NULL, // nada que liberar
    };

    ss = selector_register(selector, server, &pop3filter, OP_READ, NULL);

    if(ss != SELECTOR_SUCCESS) {
        err_msg = "registering fd";
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

    pop3filter_pool_destroy();//TODO: also necesitamos esto en el nio

    if(server >= 0) {
        close(server);
    }
    return ret;
}