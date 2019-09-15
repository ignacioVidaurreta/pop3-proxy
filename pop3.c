#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <pthread.h>
#include <netinet/in.h> //TODO(Nachito): Ver si lo puedo remover para evitar doble include
#include <arpa/inet.h>

#include "include/pop3.h"
#include "include/parser.h"
#include "include/server.h"
#include "include/client.h"
#include "include/config.h"
#include "include/logger.h"

#define N(x) (sizeof(x)/sizeof((x)[0]))
#define LOCALHOST "127.0.0.1"



extern struct config *options;

struct state_manager *state;

struct metrics_manager * metrics;

int clean_up(int fd, int origin_fd, int failed){
    if(origin_fd != 1){
        close(origin_fd);
    }
    close(fd);

    return !failed;
}

void init_state_manager() {
    state = malloc(sizeof(struct state_manager));
    state->state = RESPONSE;
    state->is_single_line = TRUE;
    state->found_CR = FALSE;
    state->found_LF = FALSE;
    state->found_dot = FALSE;
}

void init_metrics_manager() {
    metrics = malloc(sizeof(struct metrics_manager));
    metrics->concurrent_connections = 0;
    metrics->number_of_connections = 0;
    metrics->transfered_bytes = 0;
}

/**
 * maneja cada conexión entrante
 *
 * @param fd     descriptor de la conexión entrante.
 * @param caddr  información de la conexiónentrante.
 */
static void POP3_handle_connection(const int fd, const struct sockaddr* clientAddress){
    logger(INFO, "Connection established with a client.", get_time());
    const int server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    init_state_manager();
    init_metrics_manager();

    if(server_fd < 0){
        fprintf(stderr,"Cannot connect to POP3 server\n");
        return;
    }

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;

    int ret = inet_pton(AF_INET, LOCALHOST, &server_address.sin_addr.s_addr);
    if( ret == 0){
        print_error("inet_pton() failed: Invalid network address", get_time());
        return;
    }else if (ret < 0){
        print_error("inet_pton() failed: Invalid address family", get_time());
        return;
    }
    server_address.sin_port = htons((in_port_t) options->origin_port);

    //Connect to POP3 server
    if(connect(server_fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0){
        print_error("Connection to POP3 Server failed", get_time());
        return;
    }

    char buffer[BUFFER_MAX_SIZE];

    while(state->state != END){
        switch(state->state) {
            case RESPONSE:
                memset(buffer,0,BUFFER_MAX_SIZE);
                read_from_server(server_fd, buffer);
                parse_response(buffer);
                write_response(fd, buffer);//TODO: Rename fd 3 client_fd
                break;
            case REQUEST:
                read_command(fd, buffer); 
                parse_command(buffer);
                write_to_server(server_fd, buffer);
                break;
            default: 
                print_error("Proxy entered into invalid state.", get_time());
                break;
        }
    }

    print_metrics();
    free_resources();
    close(fd);
    close(server_fd);
    exit(0);
}

/**
 * Estructura utilizada para transportar datos entre el hilo
 * que acepta sockets y los hilos que procesa cada conexión
 */
struct connection {
    int fd;
    socklen_t addrlen;
    struct sockaddr_in6 addr;
};

// Rutina de cada worker
void* handle_connection_pthread(void* args){
    const struct connection *c = args;

    // Desatachear para liberar los recursos una vez que termine el proceso
    // Esto significa que no puede ser joineado con otros threads.
    pthread_detach(pthread_self());

    POP3_handle_connection(c-> fd, (struct sockaddr*) &c->addr);
    free(args);

    return 0;
}

int serve_POP3_concurrent_blocking(const int server){
    while(1){
        struct sockaddr_in6 client_address;
        socklen_t client_address_len = sizeof(client_address);
        // Wait for client to connect
        log_port("Waiting for conneciton on port", options->origin_port);

        const int client = accept(server, (struct sockaddr*)&client_address, &client_address_len);
        if( client < 0){
            print_error("Unable to accept incoming socket", get_time());
        }else{
            struct connection* c = malloc(sizeof(struct connection));
            if (c == NULL){
                // lo trabajamos iterativamente
                POP3_handle_connection(client, (struct sockaddr*)&client_address);
            }else{
                pthread_t tid;
                c -> fd      = client;
                c -> addrlen = client_address_len;
                memcpy(&(c->addr), &client_address, client_address_len);
                if (pthread_create(&tid, 0, handle_connection_pthread, c)) {
                    free(c);
                    //lo trabajamos iterativamente
                    POP3_handle_connection(client, (struct sockaddr*)&client_address);
                }
            }
            //free(c); TODO: esto rompe el tp pero habria que liberar la connection. O no ...
        }
    }
}

void free_resources() {
    free(state);
}

void print_metrics() {
    fprintf(stdout,"\n[METRICS]\nconcurrent_connections: %d\nnumber_of_connections: %d\ntransfered_bytes: %d\n",metrics->concurrent_connections,
    metrics->number_of_connections, metrics->transfered_bytes);
}
