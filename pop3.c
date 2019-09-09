#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <pthread.h>
#include <netinet/in.h> //TODO(Nachito): Ver si lo puedo remover para evitar doble include

#include <pthread.h>

#include "include/pop3.h"
#include "include/request.h"

#define N(x) (sizeof(x)/sizeof((x)[0]))


/**
 * maneja cada conexión entrante
 *
 * @param fd     descriptor de la conexión entrante.
 * @param caddr  información de la conexiónentrante.
 */
static void POP3HandleConnection(const int fd, const struct sockaddr* clientAddress){
    /*
    uint8_t method = SOCKS_HELLO_NO_ACCEPTABLE_METHODS;
    struct request request;
    struct sockaddr *originAddress = 0x00;
    socklen_t originAddressLen     = 0;
    int originDomain;
    int originFd = -1;
    uint8_t buff[10];
    */
   //TODO(team) --> Terminar POP3HandleConnection
   printf("WIP");

    


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
static void* handleConnectionPthread(void* args){
    const struct connection *c = args;

    // Desatachear para liberar los recursos una vez que termine el proceso
    // Esto significa que no puede ser joineado con otros threads.
    pthread_detach(pthread_self());

    POP3HandleConnection(c-> fd, (struct sockaddr*) &c->addr);
    free(args);

    return 0;
}

int servePOP3ConcurrentBlocking(const int server){
    while(1){
        struct sockaddr_in6 clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        // Wait for client to connect
        const int client = accept(server, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if( client < 0){
            perror("Unable to accept incoming socket");
        }else{
            struct connection* c = malloc(sizeof(struct connection));
            if (c == NULL){
                // lo trabajamos iterativamente
                POP3HandleConnection(client, (struct sockaddr*)&clientAddr);
            }else{
                pthread_t tid;
                c -> fd      = client;
                c -> addrlen = clientAddrLen;
                memcpy(&(c->addr), &clientAddr, clientAddrLen);
                if (pthread_create(&tid, 0, handleConnectionPthread, c)) {
                    free(c);
                    //lo trabajamos iterativamente
                    POP3HandleConnection(client, (struct sockaddr*)&clientAddr);
                }
            }
        }
    }
}