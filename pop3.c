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

#define TRUE 1
#define FALSE 0

extern struct config *options;

int clean_up(int fd, int origin_fd, int failed){
    if(origin_fd != 1){
        close(origin_fd);
    }
    close(fd);

    return !failed;
}

/**
 * maneja cada conexión entrante
 *
 * @param fd     descriptor de la conexión entrante.
 * @param caddr  información de la conexiónentrante.
 */
static void POP3_handle_connection(const int fd, const struct sockaddr* clientAddress){

    const int server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

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

    int ended = FALSE;
    char response[100];
    //Initial connection to server
    read_from_server(server_fd, response);
    write_response(fd, response);

    while(!ended){
        char command[100]; //TODO(Nachito): Change hardcoded size to actual significant value
        memset(response, 0, 100);
        memset(command, 0, 100);
        read_command(fd, command);

        write_to_server(server_fd, command);
        read_from_server(server_fd, response);
        write_response(fd, response);

        if(strcmp(command, "QUIT\n") == 0){
            ended = TRUE;
        }
    }
    //close(fd);
    //close(server_fd);
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
        }
    }
}
