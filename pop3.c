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

#include "include/pop3.h"
#include "include/request.h"
#include "include/buffer.h"
#include "include/netutils.h"
#include "include/hello.h"

#define N(x) (sizeof(x)/sizeof((x)[0]))


/**
 * Punto de entrada de hilo que copia el contenido de un fd a otro.
 */
static void *
copy_pthread(void *d) {
  const int *fds = d;
//  pthread_detach(pthread_self());

  sock_blocking_copy(fds[0], fds[1]);
  shutdown(fds[0], SHUT_RD);
  shutdown(fds[1], SHUT_WR);

  return 0;
}

/** callback del parser utilizado en `read_hello' */
static void
on_hello_method(struct hello_parser *p, const uint8_t method) {
    uint8_t *selected  = p->data;

    if(SOCKS_HELLO_NOAUTHENTICATION_REQUIRED == method) {
       *selected = method;
    }
}

/**
 * lee e interpreta la trama `hello' que arriba por fd
 *
 * @return true ante un error.
 */
/*
static bool
read_hello(const int fd, const uint8_t *method) {
    // 0. lectura del primer hello
    uint8_t buff[256 + 1 + 1];
    buffer buffer; buffer_init(&buffer, N(buff), buff);
    struct hello_parser hello_parser = {
        .data                     = (void *)method,
        .on_authentication_method = on_hello_method,
    };
    hello_parser_init(&hello_parser);
    bool error = false;
    size_t buffsize;
    ssize_t n;
    do {
        uint8_t *ptr = buffer_write_ptr(&buffer, &buffsize);
        n = recv(fd, ptr, buffsize, 0);
        if(n > 0) {
            buffer_write_adv(&buffer, n);
            const enum hello_state st = hello_consume(&buffer, &hello_parser, &error);
            if(hello_is_done(st, &error)) {
                break;
            }
        } else {
            break;
        }
    } while(true);

    if(!hello_is_done(hello_parser.state, &error)) {
        error = true;
    }
    hello_parser_close(&hello_parser);
    return error;
}
*/
/**
 * lee e interpreta la trama `request' que arriba por fd
 *
 * @return true ante un error.
 */
/*
static bool
read_request(const int fd, struct request *request) {
    uint8_t buff[22];
    buffer  buffer; buffer_init(&buffer, N(buff), buff);
    size_t  buffsize;

    struct request_parser request_parser = {
        .request = request,
    };
    request_parser_init(&request_parser);
    unsigned    n = 0;
       bool error = false;

    do {
        uint8_t *ptr = buffer_write_ptr(&buffer, &buffsize);
        n = recv(fd, ptr, buffsize, 0);
        if(n > 0) {
            buffer_write_adv(&buffer, n);
            const enum request_state st = request_consume(&buffer,
                        &request_parser, &error);
            if(request_is_done(st, &error)) {
                break;
            }

        } else {
            break;
        }
    }while(true);
    if(!request_is_done(request_parser.state, &error)) {
        error = true;
    }
    request_close(&request_parser);
    return error;
}
*/
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
static void POP3HandleConnection(const int fd, const struct sockaddr* clientAddress){

    //uint8_t method = SOCKS_HELLO_NO_ACCEPTABLE_METHODS;
    struct request request;
    struct sockaddr *originAddress = 0x00;
    socklen_t originAddressLen     = 0;
    int originDomain;
    int originFd = -1;
    uint8_t buff[10];
    buffer b;
    bufferInit(&b, N(buff), (buff));

    send(fd, "+OK Welcome\r\n", strlen("+OK Welcome\r\n"), 0);
    
    // if(sockBlockingWrite(fd, &b)){
    //     return cleanUp(fd, originFd, 1);
    // }
    /*
    if(SOCKS_HELLO_NO_ACCEPTABLE_METHODS == method){
        return cleanUp(fd, originFd, 1);
    }
    */

    //2. Lectura del request 
    // enum socksResponseStatus status = statusGeneralSOCKSServerFailure;
    // if(readRequest(fd, &request)){
    //     status = statusGeneralSOCKSServerFailure;
    // }else{
    //     // 3. procesamietno
    //     switch(request.cmd){
    //         case socks_req_cmd_connect: {
    //             printf("Connected");
    //             bool error = false;
    //             status = cmd_resolve(&request, &originAddress, &originAddressLen, &originDomain);
    //             if(originAddress == NULL)
    //                 error = true;
    //             else{
    //                 originFd = socket(originDomain, SOCK_STREAM, 0);
    //                 if( originFd == -1)
    //                     error = true;
    //                 else{
    //                     if (connect(originFd , originAddress, originAddressLen) == -1){
    //                         status = errno_to_socks(errno);
    //                         error = true;
    //                     }else{
    //                         status = statusSucceeded;
    //                     }
    //                 }
    //             }
    //             if (error){
    //                 if(originFd != -1){
    //                     close(originFd);
    //                     originFd = -1;
    //                 }
    //                 close(fd);
    //             }
    //             break;
    //         }
    //         case socks_req_cmd_bind:
    //         case socks_req_cmd_associate:
    //         default:
    //             status = statusCommandNotSupported;
    //             break;
    //     }
    // }

    // //4. envío de respuesta al request
    // request_marshall(&b, status);
    // if (sockBlockingWrite(fd, &b) || originFd == -1)
    //     return cleanUp(fd, originFd, 1);

    // // 5. Copia dos vías
    // pthread_t tid;
    // int fds[2][2] = {
    //     {originFd, fd      },
    //     {fd      , originFd},
    // };

    // if(pthread_create(&tid, NULL, copyPthread, fds[0]))
    //     return cleanUp(fd, originFd, 1);
    
    // sock_blocking_copy(fds[1][0], fds[1][1]);
    // pthread_join(tid, 0);

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
void* handleConnectionPthread(void* args){
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