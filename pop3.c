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

#define N(x) (sizeof(x)/sizeof((x)[0]))
#define LOCALHOST "127.0.0.1"
#define SERVER_LISTEN_PORT 110

int cleanUp(int fd, int originFd, int failed){
    if(originFd != 1){
        close(originFd);
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
        fprintf(stderr, "inet_pton() failed: Invalid network address");
        return;
    }else if (ret < 0){
        fprintf(stderr,"inet_pton() failed: Invalid address family");
        return;
    }
    server_address.sin_port = htons((in_port_t) SERVER_LISTEN_PORT);

    //Connect to POP3 server
    if(connect(server_fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0){
        fprintf(stderr,"Connection to POP3 Server failed");
        return;
    }

    //TODO(TEAM) A partir de acá, ya tenemos la mecánica de conexión (WIJUU FINALLY)
    //Ahora tenemos que ver cómo vamos a hacer para manejar los comandos y todo.
    fprintf(stdout, "[WIP]\n");
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
void* handleConnectionPthread(void* args){
    const struct connection *c = args;

    // Desatachear para liberar los recursos una vez que termine el proceso
    // Esto significa que no puede ser joineado con otros threads.
    pthread_detach(pthread_self());

    POP3_handle_connection(c-> fd, (struct sockaddr*) &c->addr);
    free(args);

    return 0;
}

int servePOP3ConcurrentBlocking(const int server){
    while(1){
        struct sockaddr_in6 clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        // Wait for client to connect
        fprintf(stdout, "Waiting for conneciton on port %d\n", SERVER_LISTEN_PORT);
        const int client = accept(server, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if( client < 0){
            perror("Unable to accept incoming socket");
        }else{
            struct connection* c = malloc(sizeof(struct connection));
            if (c == NULL){
                // lo trabajamos iterativamente
                POP3_handle_connection(client, (struct sockaddr*)&clientAddr);
            }else{
                pthread_t tid;
                c -> fd      = client;
                c -> addrlen = clientAddrLen;
                memcpy(&(c->addr), &clientAddr, clientAddrLen);
                if (pthread_create(&tid, 0, handleConnectionPthread, c)) {
                    free(c);
                    //lo trabajamos iterativamente
                    POP3_handle_connection(client, (struct sockaddr*)&clientAddr);
                }
            }
        }
    }
}
