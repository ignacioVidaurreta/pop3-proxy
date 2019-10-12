/**
 * pop3nio.c  - controla el flujo de un proxy POP3 con sockets no bloqueantes
 */
#include<stdio.h>
#include <stdlib.h>  // malloc
#include <string.h>  // memset
#include <assert.h>  // assert
#include <errno.h>
#include <time.h>
#include <unistd.h>  // close
#include <pthread.h>

#include <arpa/inet.h>

#include "request.h"
#include "buffer.h"

#include "stm.h"
#include "pop3nio.h"
#include"netutils.h"
#include "include/pop3nio.h"
#define N(x) (sizeof(x)/sizeof((x)[0]))

/** maquina de estados general */
enum pop3_state {
   /**
     * Se conecta con el servidor de origen.
     *
     * Transiciones:
     *   - CAPA     mientras el mensaje no esté completo
     *   - ERROR    ante cualquier error de conexion
     */
    CONNECTING,

    /**
     * Recibe el mensaje de bienvenida del servidor de origen
     *
     * Transiciones:
     *   - CAPA     mientras el mensaje no esté completo
     *   - ERROR    ante cualquier error (IO/parseo)
     */
    EHLO,

    /**
     * Comprueba que el servidor origen tenga capacidad de pipelining
     *
     * Transiciones:
     *   - REQUEST       mientras el mensaje no esté completo
     *   - RESPONSE      si el mensajte esta completo y transmitido por completo
     *   - ERROR         ante cualquier error (IO/parseo)
     */
    CAPA,


    /**
     * Recibe un pedido del cliente y lo transmite al host
     *
     * Transiciones:
     *   - REQUEST       mientras el mensaje no esté completo
     *   - RESPONSE      si el mensajte esta completo y transmitido por completo
     *   - ERROR         ante cualquier error (IO/parseo)
     */
    REQUEST,

    /**
     * Recibe una respuesta del servidor origen y la procesa
     * 
     * Transiciones:
     *     - RESPONSE_RECV        mientras la respuesta no este completa
     *     - FILTER               si se recibe un mail para filtrar
     *     - RESPONSE_SEND        si la respuesta no requiere procesamiento
     *     - ERROR                ante cualquier error (IO/parseo)
     */
    RESPONSE_RECV,

    /**
     * Envia la respuesta del servidor origen al cliente
     * 
     * Transiciones:
     *    - RESPONSE_SEND    mientras el mensaje no se haya enviado por completo
     *    - REQUEST          una vez finalizado el envio del mensaje
     *    - ERROR            ante cualquier error (IO/parseo)
     */
    RESPONSE_SEND,

    /**
     * envia el contenido del mail recivido al filtro para ser procesado
     *
     * Transiciones:
     *   - RESPONSE_SEND  una vez que el mensaje haya sido filtrado
     *   - ERROR        ante error durante la ejecucion/conexion con de filtro.
     */
    FILTER,

    // estados terminales
    DONE,
    ERROR,
};

/*
 * Si bien cada estado tiene su propio struct que le da un alcance
 * acotado, disponemos de la siguiente estructura para hacer una única
 * alocación cuando recibimos la conexión.
 *
 * Se utiliza un contador de referencias (references) para saber cuando debemos
 * liberarlo finalmente, y un pool para reusar alocaciones previas.
 */
struct pop3 {
    /** información del cliente */
    struct sockaddr_storage       client_addr;
    socklen_t                     client_addr_len;
    int                           client_fd;

    char*                         client_username;

    /** resolución de la dirección del origin server */
    struct addrinfo              *origin_resolution;
    /** intento actual de la dirección del origin server */
    struct addrinfo              *origin_resolution_current;

    /** información del origin server */
    struct sockaddr_storage       origin_addr;
    socklen_t                     origin_addr_len;
    int                           origin_domain;
    int                           origin_fd;

    /** maquinas de estados */
    struct state_machine          stm;

    /** estados para el client_fd */
    union {
        struct request_st         request;
        struct response_send_st   response_send;
        struct error_st           error;
    } client;

    /** estados para el origin_fd */
    union {
        struct ehlo_st            ehlo;
        struct capa_st            capa;
        struct request_st         conn;
        struct response_recv      response_recv;
    } orig;

    /** estados para el filter_fd */
    union {
        struct filter_st          filter;
    } filter;

    /** buffers para ser usados read_buffer, write_buffer.*/
    uint8_t raw_buff_a[2048], raw_buff_b[2048]; //Si neecsitamos mas buffers, los podemos agregar aca.
    buffer read_buffer, write_buffer;
    
    /** cantidad de referencias a este objeto. si es uno se debe destruir */
    unsigned references;

    /** siguiente en el pool */
    struct pop3 *next;
};

/**
 * Pool de `struct pop3', para ser reusados.
 *
 * Como tenemos un unico hilo que emite eventos no necesitamos barreras de
 * contención.
 */

static const unsigned  max_pool  = 50; // tamaño máximo
static unsigned        pool_size = 0;  // tamaño actual
static struct socks5 * pool      = 0;  // pool propiamente dicho

static const struct state_definition *
pop3_describe_states(void);

/** crea un nuevo `struct pop3' */
static struct pop3 *
pop3_new(int client_fd) {
    struct pop3 *ret;

    if(pool == NULL) {
        ret = malloc(sizeof(*ret));
    } else {
        ret       = pool;
        pool      = pool->next;
        ret->next = 0;
    }
    if(ret == NULL) {
        goto finally;
    }
    memset(ret, 0x00, sizeof(*ret));

    ret->origin_fd       = -1;
    ret->client_fd       = client_fd;
    ret->client_addr_len = sizeof(ret->client_addr);

    ret->stm    .initial   = CONNECTING;
    ret->stm    .max_state = ERROR;
    ret->stm    .states    = pop3_describe_states();
    stm_init(&ret->stm);

    buffer_init(&ret->read_buffer,  N(ret->raw_buff_a), ret->raw_buff_a);
    buffer_init(&ret->write_buffer, N(ret->raw_buff_b), ret->raw_buff_b);

    //TODO: INCREMENTAR CANTIDAD DE CONEXIONES CONCURRENTES, 
    //TODAVIA NO TENEMOS LA STRUCT GLOBAL METRICS REFERENCIADA EN ESTE ARCHIVO

    ret->references = 1;
finally:
    return ret;
}

void pop3filter_passive_accept{
    //https://stackoverflow.com/questions/16010622/reasoning-behind-c-sockets-sockaddr-and-sockaddr-storage
    struct sockaddr_storage       client_addr;
    socklen_t                     client_addr_len = sizeof(client_addr);
    struct pop3             *state           = NULL; //TODO structure

    const int client = accept(key->fd, (struct sockaddr*) &client_addr,
                                                        &client_addr_len);
    if(client == -1) {
        goto fail;
    }
    if(selector_fd_set_nio(client) == -1) {
        goto fail;
    }
    state = pop3_new(client); //Todo initialization of structure
    if(state == NULL) {
        // sin un estado, nos es imposible manejaro.
        // tal vez deberiamos apagar accept() hasta que detectemos
        // que se liberó alguna conexión.
        goto fail;
    }
    memcpy(&state->client_addr, &client_addr, client_addr_len);
    state->client_addr_len = client_addr_len;

    //TODO create pop3_handler
    if(SELECTOR_SUCCESS != selector_register(key->s, client, &socks5_handler,
                                            OP_READ, state)) {
        goto fail;
    }
    return ;
fail:
    if(client != -1) {
        close(client);
    }
    socks5_destroy(state);
}

/** definición de handlers para cada estado */
static const struct state_definition client_statbl[] = {
    {
        .state            = CONNECTING,
        .on_arrival       = connection_init,
    }, {
        .state            = EHLO,
        .on_arrival       = ehlo_ready,
        .on_read_ready    = capa_read,
    },{
        .state            = CAPA,
        .on_arrival       = capa_init,
        .on_read_ready    = capa_read,
    },{
        .state            = REQUEST,
        .on_read_ready    = request_read,
        .on_depature      = request_sent,
    },{
        .state            = RESPONSE_RECV,
        .on_read_ready    = response_read,
    },{
        .state            = RESPOND_SEND,
        .on_write_ready   = response_send,
    }, {
        .state            = FILTER,
        .on_arrival       = filter_init,
        .on_write_ready   = filter_send,
        .on_read_ready    = filter_recv,
    }, {
        .state            = DONE,
    },{
        .state            = ERROR,
    }
};

static const struct state_definition *
pop3_describe_states(void) {
    return client_statbl;
}