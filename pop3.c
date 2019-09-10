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
#include "include/buffer.h"
#include "include/netutils.h"

#define N(x) (sizeof(x)/sizeof((x)[0]))

/**
 * Punto de entrada de hilo que copia el contenido de un fd a otro.
 */
static void *
copyPthread(void *d) {
  const int *fds = d;

  sock_blocking_copy(fds[0], fds[1]);
  shutdown(fds[0], SHUT_RD);
  shutdown(fds[1], SHUT_WR);

  return 0;
}

static int cleanUp(int fd, int originFd, int failed){
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
static void POP3HandleConnection(const int fd, const struct sockaddr* clientAddress){

    uint8_t method = SOCKS_HELLO_NO_ACCEPTABLE_METHODS;
    struct request request;
    struct sockaddr *originAddress = 0x00;
    socklen_t originAddressLen     = 0;
    int originDomain;
    int originFd = -1;
    uint8_t buff[10];
    buffer b;
    bufferInit(&b, N(buff), (buff));

    // Leida del hello enviado por el cliente
    if(readHello(fd, &method)){
        return cleanUp(fd, originFd, 1);
    }
    //1. Envío de la respuesta
    const uint8_t r = (method == SOCKS_HELLO_NO_ACCEPTABLE_METHODS)? 0xFF: 0x00;
    helloMarshall(&b, r);

    if(sockBlockingWrite(fd, &b)){
        return cleanUp(fd, originFd, 1);
    }
    if(SOCKS_HELLO_NO_ACCEPTABLE_METHODS == method){
        return cleanUp(fd, originFd, 1);
    }

    //2. Lectura del request 
    enum socksResponseStatus status = statusGeneralSOCKSServerFailure;
    if(readRequest(fd, &request)){
        status = statusGeneralSOCKSServerFailure;
    }else{
        // 3. procesamietno
        switch(request.cmd){
            case socks_req_cmd_connect: {
                bool error = false;
                status = cmd_resolve(&request, &originAddress, &originAddressLen, &originDomain);
                if(originAddress == NULL)
                    error = true;
                else{
                    originFd = socket(originDomain, SOCK_STREAM, 0);
                    if( originFd == -1)
                        error = true;
                    else{
                        if (connect(originFd , originAddress, originAddressLen) == -1){
                            status = errno_to_socks(errno);
                            error = true;
                        }else{
                            status = statusSucceeded;
                        }
                    }
                }
                if (error){
                    if(originFd != -1){
                        close(originFd);
                        originFd = -1;
                    }
                    close(fd);
                }
                break;
            }
            case socks_req_cmd_bind:
            case socks_req_cmd_associate:
            default:
                status = statusCommandNotSupported;
                break;
        }
    }

    //4. envío de respuesta al request
    request_marshall(&b, status);
    if (sockBlockingWrite(fd, &b) || originFd == -1)
        return cleanUp(fd, originFd, 1);

    // 5. Copia dos vías
    pthread_t tid;
    int fds[2][2] = {
        {originFd, fd      },
        {fd      , originFd},
    };

    if(pthread_create(&tid, NULL, copyPthread, fds[0]))
        return cleanUp(fd, originFd, 1);
    
    sock_blocking_copy(fds[1][0], fds[1][1]);
    pthread_join(tid, 0);

    return cleanUp(fd, originFd, 0);

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