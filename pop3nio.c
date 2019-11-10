#include <stdio.h>
#include <stdlib.h>  // malloc
#include <string.h>  // memset
#include <strings.h> // strncasecmp
#include <assert.h>  // assert
#include <errno.h>
#include <unistd.h>  // close
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/wait.h>
#include <pthread.h>
#include <poll.h>
#include <metrics.h>

#include "include/buffer.h"
#include "include/stm.h"
#include "include/pop3nio.h"
#include "include/selector.h"
#include "include/server.h"
#include "include/parser.h"
#include "include/pop3.h"
#include "include/logger.h"
#include "include/config.h"
#include "include/client.h"

#define N(x) (sizeof(x)/sizeof((x)[0]))
/** obtiene el struct (socks5 *) desde la llave de selección  */
#define ATTACHMENT(key) ( (struct pop3 *)(key)->data)

extern struct config *options;

extern struct metrics_manager *metrics;

/**
 * Pool de `struct pop3', para ser reusados.
 *
 * Como tenemos un unico hilo que emite eventos no necesitamos barreras de
 * contención.
 */

// static const unsigned  max_pool  = 50; // tamaño máximo
// static unsigned        pool_size = 0;  // tamaño actual
static struct pop3     *pool     = 0;  // pool propiamente dicho

static const struct state_definition *
pop3_describe_states(void);

/** crea un nuevo `struct pop3' */
static struct pop3 * 
pop3_new(int client_fd){
    struct pop3 *ret;

    if(pool == NULL) {
        ret = malloc(sizeof(*ret));
    }else {
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

    ret->stm    .initial   = RESOLVE;
    ret->stm    .max_state = ERROR;
    ret->stm    .states    = pop3_describe_states();
    stm_init(&ret->stm);

    buffer_init(&ret->read_buffer,  N(ret->raw_buff_a), ret->raw_buff_a);
    buffer_init(&ret->write_buffer, N(ret->raw_buff_b), ret->raw_buff_b);
    buffer_init(&ret->request_buffer, N(ret->raw_buff_c), ret->raw_buff_c);
    buffer_init(&ret->cmd_request_buffer, N(ret->raw_buff_d), ret->raw_buff_d);

    metrics->concurrent_connections++;
    //TODAVIA NO TENEMOS LA STRUCT GLOBAL METRICS REFERENCIADA EN ESTE ARCHIVO

    ret->references = 1;
finally:
    return ret;
}

static ssize_t send_next_request(struct selector_key *key, buffer *b) {
    buffer *cb            = ATTACHMENT(key)->client.request.cmd_buffer;
    uint8_t *cptr;

    size_t count;
    ssize_t n = 0;

    size_t i = 0;

    if (!buffer_can_read(b))
        return 0;

    while (buffer_can_read(b) && n == 0) {
        i++;
        char c = buffer_read(b);
        if (c == '\n') {
            n = i;
        }
        buffer_write(cb, c);
    }

    if (n == 0) {
        return 0;
    }

    cptr = buffer_read_ptr(cb, &count);

    int cmd_n;
    char cmd[16], arg1[32], arg2[32], extra[5];
    char * aux = malloc(count+1);
    memcpy(aux, cptr, count);
    aux[count] = '\0';
    cmd_n = sscanf(aux, "%s %s %s %s", cmd, arg1, arg2, extra);

    assign_cmd(key, cmd, cmd_n);

    free(aux);

    if (ATTACHMENT(key)->has_pipelining) {
        struct next_request *next = malloc(sizeof(next));
        next->cmd_type = ATTACHMENT(key)->client.request.cmd_type;
        next->next = NULL;

        if (ATTACHMENT(key)->client.request.last_cmd_type == NULL) {
            ATTACHMENT(key)->client.request.next_cmd_type = next;
            ATTACHMENT(key)->client.request.last_cmd_type = next;
        }
        else {
            ATTACHMENT(key)->client.request.last_cmd_type->next = next;
            ATTACHMENT(key)->client.request.last_cmd_type = next;
        }

        buffer *ab = ATTACHMENT(key)->client.request.aux_buffer;
        uint8_t *aptr;
        size_t acount;

        aptr = buffer_write_ptr(ab, &acount);
        memcpy(aptr, cptr, count);
        buffer_write_adv(ab, count);
    }
    else {
        n = send(ATTACHMENT(key)->origin_fd, cptr, count, MSG_NOSIGNAL);
    }

    buffer_reset(cb);

    return n;  
}

/* declaración forward de los handlers de selección de una conexión
 * establecida entre un cliente y el proxy.
 */
static void pop3_read   (struct selector_key *key);
static void pop3_write  (struct selector_key *key);
static void pop3_block  (struct selector_key *key);
static void pop3_close  (struct selector_key *key);
static const struct fd_handler pop3_handler = {
    .handle_read   = pop3_read,
    .handle_write  = pop3_write,
    .handle_close  = pop3_close,
    .handle_block  = pop3_block,
};

void pop3_destroy(struct pop3* state){
    //Do Nothing
}
void pop3filter_passive_accept(struct selector_key* key){
    //https://stackoverflow.com/questions/16010622/reasoning-behind-c-sockets-sockaddr-and-sockaddr-storage
    struct sockaddr_storage       client_addr;
    socklen_t                     client_addr_len = sizeof(client_addr);
    struct pop3             *state           = NULL;

    const int client = accept(key->fd, (struct sockaddr*) &client_addr,
                                                        &client_addr_len);
    if(client == -1) {
        goto fail;
    }
    if(selector_fd_set_nio(client) == -1) {
        goto fail;
    }
    state = pop3_new(client);
    if(state == NULL) {
        // sin un estado, nos es imposible manejaro.
        // tal vez deberiamos apagar accept() hasta que detectemos
        // que se liberó alguna conexión.
        goto fail;
    }
    memcpy(&state->client_addr, &client_addr, client_addr_len);
    state->client_addr_len = client_addr_len;

    if(SELECTOR_SUCCESS != selector_register(key->s, client, &pop3_handler,
                                            OP_WRITE, state)) {
        goto fail;
    }
    return ;
fail:
    if(client != -1) {
        close(client);
    }
    pop3_destroy(state);
}

static void response_init(const unsigned state, struct selector_key *key){
    struct response_st *d = &ATTACHMENT(key)->origin.response;

    d->rb = &(ATTACHMENT(key)->read_buffer);
    d->wb = &(ATTACHMENT(key)->write_buffer);
}

static unsigned response_read(struct selector_key *key){
    struct response_st *d = &ATTACHMENT(key)->origin.response;
    enum pop3_state ret      = RESPONSE;

    buffer  *b         = d->rb;
    uint8_t *ptr;
    size_t  count;
    ssize_t  n;

    ptr = buffer_write_ptr(b, &count);
    n = recv(key->fd, ptr, count, 0);

    if(n > 0) {
        //TODO: Multiline parse email (connect with old one)
        buffer_write_adv(b, n);
        selector_status ss = SELECTOR_SUCCESS;
        ss |= selector_set_interest_key(key, OP_NOOP);
        ss |= selector_set_interest(key->s, ATTACHMENT(key)->client_fd, OP_WRITE);
        ret = ss == SELECTOR_SUCCESS ? RESPONSE : ERROR;

    } else {
        ret = ERROR;
    }

    if(ret == ERROR) {
        perror("Error reading file");
        //print_error_message_with_client_ip(ATTACHMENT(key)->client_addr, "error reading response from origin server");
    }

    return ret;
}
static ssize_t
send_to_server(struct selector_key *key, buffer * b) {
    buffer *sb            = ATTACHMENT(key)->origin.response.wb;
    uint8_t *sptr;

    size_t count;
    ssize_t n = 0;

    size_t i = 0;

    while (buffer_can_read(b) && n == 0) {
        i++;
        char c = buffer_read(b);
        if (c == '\n') {
            n = i;
        }
        buffer_write(sb, c);
    }

    if (n == 0) {
        return 0;
    }

    sptr = buffer_read_ptr(sb, &count);

    if (strncasecmp((char*)sptr, "+OK", 3) != 0) {
        ATTACHMENT(key)->client.request.cmd_type = DEFAULT;
    }

    n = send(ATTACHMENT(key)->client_fd, sptr, count, MSG_NOSIGNAL);

    buffer_reset(sb);

    return n;
}
/** Escribe la respuesta en el cliente */
static unsigned response_write(struct selector_key *key){
    struct response_st *d = &ATTACHMENT(key)->origin.response;
    enum pop3_state ret = RESPONSE;

    buffer  *b = d->rb;
    ssize_t  n;

    n = send_to_server(key, b);

    //TODO: QUIT
    if (n == -1) {
        ret = ERROR;
    } else if (n == 0) {
        selector_status ss = SELECTOR_SUCCESS;
        ss |= selector_set_interest_key(key, OP_NOOP);
        ss |= selector_set_interest(key->s, ATTACHMENT(key)->origin_fd, OP_READ);
        ret = SELECTOR_SUCCESS == ss ? RESPONSE : ERROR;
    }

    selector_status ss = SELECTOR_SUCCESS;
    ss |= selector_set_interest_key(key, OP_NOOP);
    if (buffer_can_read(b)) {
        ss |= selector_set_interest(key->s, ATTACHMENT(key)->client_fd, OP_WRITE);
        ret = ss == SELECTOR_SUCCESS ? RESPONSE : ERROR;
    }
    else {
        ss |= selector_set_interest(key->s, ATTACHMENT(key)->origin_fd, OP_WRITE);
        ret = SELECTOR_SUCCESS == ss ? REQUEST : ERROR;
    }

    return ret;
}

///////////////////////////////////////////////////////////////////////////////
// Handlers top level de la conexión pasiva.
// son los que emiten los eventos a la maquina de estados.
///////////////////////////////////////////////////////////////////////////////
static void pop3_done(struct selector_key* key);

static void pop3_read(struct selector_key *key) {
    struct state_machine *stm   = &ATTACHMENT(key)->stm;
    const enum pop3_state st = stm_handler_read(stm, key);

    if(st == ERROR || st == DONE) {
        pop3_done(key);
    }
}

static void pop3_write(struct selector_key *key) {
    struct state_machine *stm   = &ATTACHMENT(key)->stm;
    const enum pop3_state st = stm_handler_write(stm, key);

    if(st == ERROR || st == DONE) {
        pop3_done(key);
    }
}

static void pop3_block(struct selector_key *key) {
    struct state_machine *stm   = &ATTACHMENT(key)->stm;
    const enum pop3_state st = stm_handler_block(stm, key);

    if(st == ERROR || st == DONE) {
        pop3_done(key);
    }
}

static void pop3_close(struct selector_key *key) {
    pop3_destroy(ATTACHMENT(key));
}

static void pop3_done(struct selector_key* key) {
    const int fds[] = {
        ATTACHMENT(key)->client_fd,
        ATTACHMENT(key)->origin_fd,
    };
    for(unsigned i = 0; i < N(fds); i++) {
        if(fds[i] != -1) {
            if(selector_unregister_fd(key->s, fds[i]) != SELECTOR_SUCCESS ) {
                abort();
            }
            close(fds[i]);
        }
    }
}


/////////////////////////////////////////////////////////
///////////       Funciones de estados        ///////////
/////////////////////////////////////////////////////////

/**
 * Realiza la resolución de DNS bloqueante.
 *
 * Una vez resuelto notifica al selector para que el evento esté
 * disponible en la próxima iteración.
 */
static void *
connection_resolve_blocking(void *data) {
    struct selector_key *key = (struct selector_key *) data;
    struct pop3         *p   = ATTACHMENT(key);

    pthread_detach(pthread_self());
    p->origin_resolution = 0;

    struct addrinfo hints = {
            .ai_family    = AF_UNSPEC,
            .ai_socktype  = SOCK_STREAM,
            .ai_flags     = AI_PASSIVE,
            .ai_protocol  = IPPROTO_TCP,
            .ai_canonname = NULL,
            .ai_addr      = NULL,
            .ai_next      = NULL,
    };

    char buff[7];
    snprintf(buff, sizeof(buff), "%hu",options->origin_port);
    getaddrinfo(options->origin_server, buff, &hints, &p->origin_resolution);

    selector_notify_block(key->s, key->fd);

    free(data);
    return 0;
}

///////      RESOLVE      ///////

// Resolucion de nombre del origin server
static unsigned connection_resolve(struct selector_key *key) {
    unsigned ret;
    pthread_t tid;

    struct selector_key* k = malloc(sizeof(*key));
    if(k == NULL) {
        ret = ERROR;
    }
    else {
        memcpy(k, key, sizeof(*k));
        if(pthread_create(&tid, 0, connection_resolve_blocking, k) == -1) {
            ret = ERROR;
        } 
        else{
            ret = RESOLVE;
            selector_set_interest_key(key, OP_NOOP);
        }
    }
    return ret;
}



/** procesa el resultado de la resolución de nombres */
static unsigned
connection_resolve_complete(struct selector_key *key) {
    struct pop3       *pop =  ATTACHMENT(key);
    unsigned           ret = CONNECTING;

    if(pop->origin_resolution == NULL) {
        pop->error = "Couldn't resolve origin name server.";
        goto error;
    } else {
        pop->origin_domain   = pop->origin_resolution->ai_family;
        pop->origin_addr_len = pop->origin_resolution->ai_addrlen;
        memcpy(&pop->origin_addr,
               pop->origin_resolution->ai_addr,
               (size_t) pop->origin_resolution->ai_addrlen);
        pop->origin_resolution_current = pop->origin_resolution;
    }

    //OPEN SOCKET CONNECTION
    int sock = socket(pop->origin_domain, SOCK_STREAM, IPPROTO_TCP);

    if (sock < 0 || selector_fd_set_nio(sock) == -1) {
        pop->error = "Error creating socket for connectino with origin server";
        goto error;
    }

    if (connect(sock, (const struct sockaddr *)&pop->origin_addr, pop->origin_addr_len) == -1) {
        if(errno == EINPROGRESS) {
            // dejamos de de pollear el socket del cliente
            selector_status st = selector_set_interest_key(key, OP_NOOP);
            if(SELECTOR_SUCCESS != st) {
                pop->error = "Error connecting to origin server";
                goto error;
            }

            // esperamos la conexion en el nuevo socket
            st = selector_register(key->s, sock, &pop3_handler, OP_WRITE, key->data);
            if(SELECTOR_SUCCESS != st) {
                pop->error = "Error connecting to origin server";
                goto error;
            }
            pop->references += 1;
        } else {
            selector_unregister_fd(key->s, sock);
            pop->error = "Error connecting to origin server";
            goto error;
        }
    } else {
        pop->error = "Error connecting to origin server";
        goto error;
    }

    return ret;
error:
    print_error(pop->error, get_time());
    return ERROR;
}

///////      CONNECTING      ///////

static unsigned connecting(struct selector_key *key) {
    struct pop3 *p = ATTACHMENT(key);
    int error;
    socklen_t len = sizeof(error);
    
    p->origin_fd = key->fd;

    if (getsockopt(key->fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
        p->error = "Connection refused (while connecting to origin server).";
        selector_set_interest_key(key, OP_NOOP);
        return ERROR;
    } else {
        if (error == 0) {
            p->origin_fd = key->fd;
        } else {
            selector_unregister_fd(key->s, key->fd);
            close(key->fd);
            p->error = "Connection refused (while connecting to origin server).";
            return ERROR;
        }
    }

    selector_status s = SELECTOR_SUCCESS;
    
    s |= selector_set_interest_key(key, OP_READ);
    s |= selector_set_interest(key->s, p->client_fd, OP_NOOP);
    return SELECTOR_SUCCESS == s ? EHLO : ERROR;
}



///////      EHLO      ///////

static void
ehlo_init(const unsigned state, struct selector_key *key) {
    struct pop3     *p  =  ATTACHMENT(key);
    struct ehlo_st *d   = &p->origin.ehlo;
    d->rb               = &p->read_buffer;
    d->wb               = &p->write_buffer;
}

static unsigned
ehlo_read(struct selector_key *key) {
    struct pop3 *p     =  ATTACHMENT(key);
    struct ehlo_st *d  = &p->origin.ehlo;
    unsigned  ret      = EHLO;
    buffer *b          = d->rb;
    uint8_t *ptr;
    size_t  count;
    ssize_t  n;

    ptr = buffer_write_ptr(b, &count);
    n = recv(key->fd, ptr, count, 0);

    if(n > 0) {
        buffer_write_adv(b, n);
        selector_status ss = SELECTOR_SUCCESS;
        ss |= selector_set_interest_key(key, OP_NOOP);
        ss |= selector_set_interest(key->s, p->client_fd, OP_WRITE);
        if (ss != SELECTOR_SUCCESS) {
            ret = ERROR;
        }
    } else {
        ret = ERROR;
    }

    return ret;
}

static ssize_t
send_ehlo_status_line(struct selector_key *key, buffer * b) {
    buffer *wb = ATTACHMENT(key)->origin.ehlo.wb;
    uint8_t *read_ptr;

    size_t count;
    ssize_t n = 0;

    size_t i = 0;


    /* Parse hello message */
    while (buffer_can_read(b) && n == 0) {
        i++;
        char c = buffer_read(b);
        if( c != '+' && i ==0 ) {
            return -1;
        }
        if (c == '\n') {
            n = i;
        }
        buffer_write(wb, c);
    }

    if (n == 0) {
        return 0;
    }

    read_ptr = buffer_read_ptr(wb, &count);

    n = send(ATTACHMENT(key)->client_fd, read_ptr, count, MSG_NOSIGNAL);

    buffer_reset(wb);

    return n;
}

static unsigned
ehlo_write(struct selector_key *key) {
    struct pop3 *p     =  ATTACHMENT(key);
    struct ehlo_st *d = &p->origin.ehlo;

    unsigned  ret     = EHLO;
    buffer  *buffer   = d->rb;

    ssize_t  n;

    n = send_ehlo_status_line(key, buffer);

    if (n == -1) {
        ret = ERROR;
    } 
    else if (n == 0) {
        selector_status ss = SELECTOR_SUCCESS;
        ss |= selector_set_interest_key(key, OP_NOOP);
        ss |= selector_set_interest(key->s, ATTACHMENT(key)->origin_fd, OP_READ);
        ret = SELECTOR_SUCCESS == ss ? EHLO : ERROR;
    } else {
        selector_status ss = SELECTOR_SUCCESS;
        ss |= selector_set_interest_key(key, OP_NOOP);
        ss |= selector_set_interest(key->s, ATTACHMENT(key)->origin_fd, OP_WRITE);
//        ret = SELECTOR_SUCCESS == ss ? CAPA : ERROR; TODO:implement CAPA STATE
        ret = SELECTOR_SUCCESS == ss ? REQUEST : ERROR;
    }

    return ret;
}



///////      REQUEST      ///////

/** inicializa las variables del estado REQUEST */
static void request_init(const unsigned state, struct selector_key *key) {
    struct request_st * d = &ATTACHMENT(key)->client.request;
    d->buffer = &ATTACHMENT(key)->request_buffer;
    d->cmd_buffer = &ATTACHMENT(key)->cmd_request_buffer;
}

/** Lee la request del cliente */
static unsigned request_read(struct selector_key *key){
    struct request_st *d = &ATTACHMENT(key)->client.request;
    enum pop3_state ret  = REQUEST;

    buffer *buff            = d->buffer;
    uint8_t *ptr;
    size_t  count;
    ssize_t  n;

    ptr = buffer_write_ptr(buff, &count);
    n = recv(key->fd, ptr, count, 0);

    if(n > 0 || buffer_can_read(buff)) {
        buffer_write_adv(buff, n);
        selector_status ss = SELECTOR_SUCCESS;
        ss |= selector_set_interest_key(key, OP_NOOP);
        ss |= selector_set_interest(key->s, ATTACHMENT(key)->origin_fd, OP_WRITE);
        ret = SELECTOR_SUCCESS == ss ? REQUEST : ERROR;
    } else {
        ret = ERROR;
    }

    if(ret == ERROR) {
        //print_error_with_address(ATTACHMENT(key)->client_addr, "Error reading client response"); TODO(Nachito)
    }
    return ret;
}

/** Escribe la request en el server */
static unsigned request_write(struct selector_key *key) {
    struct request_st *d = &ATTACHMENT(key)->client.request;
    enum pop3_state ret      = REQUEST;

    buffer *b          = d->buffer;
    ssize_t  n;

    n = send_next_request(key, b);

    if(n == -1) {
        ret = ERROR;
    } else if (n == 0) {
        selector_status ss = SELECTOR_SUCCESS;
        ss |= selector_set_interest_key(key, OP_NOOP);
        ss |= selector_set_interest(key->s, ATTACHMENT(key)->client_fd, OP_READ);
        ret = SELECTOR_SUCCESS == ss ? REQUEST : ERROR;
    }
    else {
        if (ATTACHMENT(key)->has_pipelining) {
            if (buffer_can_read(b)) {
                if(SELECTOR_SUCCESS == selector_set_interest_key(key, OP_WRITE)) {
                    ret = REQUEST;
                } else {
                    ret = ERROR;
                }
            }
            else {
                buffer *ab            = d->aux_buffer;
                uint8_t *aptr;
                size_t  count;

                aptr = buffer_read_ptr(ab, &count);
                n = send(ATTACHMENT(key)->origin_fd, aptr, count, MSG_NOSIGNAL);
                buffer_read_adv(ab, n);

                struct next_request *aux = ATTACHMENT(key)->client.request.next_cmd_type;
                ATTACHMENT(key)->client.request.cmd_type = aux->cmd_type;
                ATTACHMENT(key)->client.request.next_cmd_type = aux->next;
                free(aux);
                if(SELECTOR_SUCCESS == selector_set_interest_key(key, OP_READ)) {
                    ret = RESPONSE;
                } else {
                    ret = ERROR;
                }
            }
        }
        else {
            if(SELECTOR_SUCCESS == selector_set_interest_key(key, OP_READ)) {
                ret = RESPONSE;
            } else {
                ret = ERROR;
            }
        }
    }

    if(ret == ERROR) {
        //print_error_message_with_client_ip(ATTACHMENT(key)->client_addr, "error writing client request to origin server");
        //TODO ERROR
    }

    return ret;
}

void assign_cmd(struct selector_key *key, char *cmd, int cmds_read){
    if (strcasecmp(cmd, "RETR") == 0) {
        ATTACHMENT(key)->client.request.cmd_type = RETR;
    }   
    else if (strcasecmp(cmd, "LIST") == 0 && cmds_read == 1) {
        ATTACHMENT(key)->client.request.cmd_type = LIST;
    }
    else if (strcasecmp(cmd, "CAPA") == 0) {
        ATTACHMENT(key)->client.request.cmd_type = CAPA;
    }
    else if (strcasecmp(cmd, "TOP") == 0) {
        ATTACHMENT(key)->client.request.cmd_type = TOP;
    }
    else if (strcasecmp(cmd, "USER") == 0) {
        ATTACHMENT(key)->client.request.cmd_type = USER;
    }
    else if (strcasecmp(cmd, "PASS") == 0) {
        ATTACHMENT(key)->client.request.cmd_type = PASS;
    }
    else if (strcasecmp(cmd, "DELE") == 0) {
        ATTACHMENT(key)->client.request.cmd_type = DELE;
    }   
    else if (strcasecmp(cmd, "QUIT") == 0) {
        ATTACHMENT(key)->client.request.cmd_type = QUIT;
    }
    else {
        ATTACHMENT(key)->client.request.cmd_type = DEFAULT;
    }   
}

static void do_nothing(const unsigned state, struct selector_key *key){
    //nothing to do
}


/** definición de handlers para cada estado */
static const struct state_definition client_statbl[] = {
    {
        .state            = RESOLVE,        
        .on_write_ready   = connection_resolve,
        .on_block_ready   = connection_resolve_complete,
    }, {
        .state            = CONNECTING,
        .on_arrival       = do_nothing,
        .on_write_ready   = connecting,
    }, {
        .state            = EHLO,
        .on_arrival       = ehlo_init,
        .on_read_ready    = ehlo_read,
        .on_write_ready   = ehlo_write,
    },{
        .state            = CAPA_STATE,
        .on_arrival       = request_init,
        .on_read_ready    = request_read,
        .on_write_ready   = request_write,
    },{
        .state            = REQUEST,
        .on_arrival       = request_init,
        .on_read_ready    = request_read,
        .on_write_ready   = request_write,
    },{
        .state            = RESPONSE,
        .on_arrival       = response_init,
        .on_read_ready    = response_read,
        .on_write_ready   = response_write,
    },
    {
        .state            = FILTER,
/*         .on_arrival       = filter_init,
        .on_write_ready   = filter_send,
        .on_read_ready    = filter_recv, */
    },
     {
        .state            = DONE,
    },{
        .state            = ERROR,
    }
};

static const struct state_definition *pop3_describe_states(void) {
    return client_statbl;
}