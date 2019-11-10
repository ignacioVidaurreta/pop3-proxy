#include <stdio.h>
#include <stdlib.h>  // malloc
#include <string.h>  // memset
#include <assert.h>  // assert
#include <errno.h>
#include <unistd.h>  // close
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/sctp.h>
#include <sys/types.h>
#include <metrics.h>


#include "include/buffer.h"
#include "include/stm.h"
#include "include/management.h"
#include "include/selector.h"
#include "include/management_cmd_parser.h"

/*
1     Byte = Command: 0=USER
                  1=PASS
                  2=METRICS
                  3=SETFILTER
2     Byte = Length: 0-256
3-... ByteContent(optional)
*/


#define N(x) (sizeof(x)/sizeof((x)[0]))
#define ATTACHMENT(key) ( (struct management *)(key)->data)

struct sctp_sndrcvinfo sndrcvinfo;
int sctp_flags;

extern struct metrics_manager *metrics;

enum management_state {
  READING_USER,
  WRITING_USER,
  READING_PASS,
  CONFIRM_PASS,
  WAITING_COMMAND,
  RESPOND_COMMAND,
  DONE,
  ERROR,
};

struct management {
    /** información del cliente */
    struct sockaddr_storage       client_addr;
    socklen_t                     client_addr_len;
    int                           client_fd;

    /** resolución de la dirección del origin server */
    struct addrinfo              *origin_resolution;

    struct state_machine          stm;
    struct request_parser parser;

    /** El resumen de la respuesta a enviar */
    enum response_status status;

    /** buffers para ser usados read_buffer, write_buffer.*/
    uint8_t raw_buff_a[2048], raw_buff_b[2048];
    buffer read_buffer, write_buffer;

    /** username del usuario que esta loggeado */
    char *username;

    /** siguiente en el pool */
    struct management *next;
};

static const struct state_definition *management_describe_states(void);
/** crea un nuevo `struct management' */
static struct management * management_new(int client_fd) {
    struct management *ret = malloc(sizeof(*ret));

    if(ret == NULL) {
        goto finally;
    }
    memset(ret, 0x00, sizeof(*ret));

    ret->client_fd       = client_fd;
    ret->client_addr_len = sizeof(ret->client_addr);

    buffer_init(&ret->read_buffer,  N(ret->raw_buff_a), ret->raw_buff_a);
    buffer_init(&ret->write_buffer, N(ret->raw_buff_b), ret->raw_buff_b);

    ret->stm    .initial   = READING_USER;
    ret->stm    .max_state = ERROR;
    ret->stm    .states    = management_describe_states();
    stm_init(&ret->stm);
finally:
    return ret;
}

static void
management_destroy(struct management* m) {
    free(m);
}

/* declaración forward de los handlers de selección de una conexión
 * establecida entre un cliente y el proxy.
 */

static void management_read   (struct selector_key *key);
static void management_write  (struct selector_key *key);
static void management_close  (struct selector_key *key);
static const struct fd_handler management_handler = {
    .handle_read   = management_read,
    .handle_write  = management_write,
    .handle_close  = management_close,
    .handle_block  = NULL,
};

/* Intenta aceptar la nueva conexión entrante*/
void
management_passive_accept(struct selector_key *key) {
    struct sockaddr_storage       client_addr;
    socklen_t                     client_addr_len = sizeof(client_addr);
    struct management             *state          = NULL;

    const int client = accept(key->fd, (struct sockaddr*) &client_addr,
                                                          &client_addr_len);
    if(client == -1) {
        goto fail;
    }
    if(selector_fd_set_nio(client) == -1) {
        goto fail;
    }
    state = management_new(client);
    if(state == NULL) {
        // sin un estado, nos es imposible manejaro.
        // tal vez deberiamos apagar accept() hasta que detectemos
        // que se liberó alguna conexión.
        goto fail;
    }
    memcpy(&state->client_addr, &client_addr, client_addr_len);
    state->client_addr_len = client_addr_len;

    if(SELECTOR_SUCCESS != selector_register(key->s, client, &management_handler,
                                              OP_READ, state)) {
        goto fail;
    }
    return ;
fail:
    if(client != -1) {
        close(client);
    }
    management_destroy(state);
}

///////////////////////////////////////////////////////////////////////////////
// Handlers top level de la conexión pasiva.
// son los que emiten los eventos a la maquina de estados.
///////////////////////////////////////////////////////////////////////////////
static void management_done(struct selector_key* key);

static void management_read(struct selector_key *key) {
    struct state_machine *stm   = &ATTACHMENT(key)->stm;
    const enum management_state st = stm_handler_read(stm, key);

    if(st == ERROR || st == DONE) {
        management_done(key);
    }
}

static void management_write(struct selector_key *key) {
    struct state_machine *stm   = &ATTACHMENT(key)->stm;
    const enum management_state st = stm_handler_write(stm, key);

    if(st == ERROR || st == DONE) {
        management_done(key);
    }
}

static void management_block(struct selector_key *key) {
    struct state_machine *stm   = &ATTACHMENT(key)->stm;
    const enum management_state st = stm_handler_block(stm, key);

    if(st == ERROR || st == DONE) {
        management_done(key);
    }
}

static void management_close(struct selector_key *key) {
    management_destroy(ATTACHMENT(key));
}

static void management_done(struct selector_key* key) {
    const int fds[] = {
        ATTACHMENT(key)->client_fd,
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

///////      USER VALIDATION      ///////

static void user_read_init(const unsigned state, struct selector_key *key) {
    struct management *management = ATTACHMENT(key);
    parser_init(&management->parser);

}

bool request_is_done(int state, bool* error ){
    if(state >= READING_ERROR && error != 0){
        *error = true;
    }

    return state >= READING_DONE;
}

static unsigned user_read(struct selector_key *key) {
    struct management *management = ATTACHMENT(key);

    buffer *read_buffer     = &management->read_buffer;
    unsigned  ret   = READING_USER;
    bool  error = false;

    uint8_t *ptr;
    size_t  count;
    ssize_t  n;

    ptr = buffer_write_ptr(read_buffer, &count);
    n = sctp_recvmsg(key->fd, ptr, count, 0, 0, &sndrcvinfo, &sctp_flags);
    if(n > 0) {
        buffer_write_adv(read_buffer, n);
        int state = parse_input(read_buffer, &management->parser, &error);
        if(request_is_done(state, 0)){
            ret = user_process(key);
        }
    } else {
        ret = ERROR;
    }

    return error ? ERROR : ret;
}

static unsigned user_process(struct selector_key *key) {
    struct management *management = ATTACHMENT(key);
    unsigned ret = WRITING_USER;

    struct spcp_request *request = &management->parser.request;
    if(management->username == NULL)
        management->username = malloc(management->parser.request.arg0_size + 1);
    else
        management->username = realloc(management->username, management->parser.request.arg0_size +1);
    if(management->username == NULL){
        management->status = READING_ERROR;
        return ERROR;
    }

    memcpy(management->username, management->parser.request.arg0, management->parser.request.arg0_size);
    management->username[management->parser.request.arg0_size] = '\0';

    if(strcmp(management->username, "admin\n") == 0) {
        management->status = CMD_SUCCESS;
        if (write_response_no_args(&management->write_buffer, 0x00) == -1) {
            ret = ERROR;
        }
    } else {
        management->status = AUTH_ERROR;
        if (write_response_no_args(&management->write_buffer, 0x01) == -1) {
            ret = ERROR;
        }
    }
    if(SELECTOR_SUCCESS != selector_set_interest_key(key, OP_WRITE)) {
        return ERROR;
    }
    return ret;
}

static unsigned user_confirm(struct selector_key *key) {
    struct management *management = ATTACHMENT(key);

    unsigned  ret     = READING_PASS;
    struct buffer *wb = &management->write_buffer;

    uint8_t *ptr;
    size_t  count;
    ssize_t  n;

    ptr = buffer_read_ptr(wb, &count);
    n = sctp_sendmsg(key->fd, ptr, count, NULL, 0, 0, 0, 0,0,0);
    if(n == -1) {
        ret = ERROR;
    } else {
        buffer_read_adv(wb, n);
        if(!buffer_can_read(wb)) {
            if(SELECTOR_SUCCESS == selector_set_interest_key(key, OP_READ)) {
                if(management->status == CMD_SUCCESS)
                    ret = READING_PASS;
                else if(management->status == AUTH_ERROR)
                    ret = READING_USER;
                else
                    ret = ERROR;
            }else
                ret = ERROR;
        }
    }

    return ret;
}

///////      PASSWORD VALIDATION      ///////

static void
pass_read_init(const unsigned state, struct selector_key *key) {
    struct management *management = ATTACHMENT(key);
    parser_init(&management->parser);
}

static unsigned pass_process(struct selector_key *key) {
    struct management *management = ATTACHMENT(key);
    unsigned ret = CONFIRM_PASS;

    char pass[management->parser.request.arg0_size + 1];
    memcpy(pass, management->parser.request.arg0, management->parser.request.arg0_size);
    pass[management->parser.request.arg0_size] = '\0';

    if(strcmp(pass, "admin\n") == 0) {
        management->status = CMD_SUCCESS;
        if (write_response_no_args(&management->write_buffer, 0x00) == -1) {
            ret = ERROR;
        }
    } else {
        management->status = AUTH_ERROR;
        if (write_response_no_args(&management->write_buffer, 0x01) == -1) {
            ret = ERROR;
        }
    }
    if(SELECTOR_SUCCESS != selector_set_interest_key(key, OP_WRITE)) {
        return ERROR;
    }
    return ret;
}

static unsigned
pass_read(struct selector_key *key) {
    struct management *management = ATTACHMENT(key);

    buffer *b     = &management->read_buffer;
    unsigned  ret   = READING_USER;
    bool  error = false;
    uint8_t *ptr;
    size_t  count;
    ssize_t  n;

    ptr = buffer_write_ptr(b, &count);
    n = sctp_recvmsg(key->fd, ptr, count, (struct sockaddr *) NULL, 0, &sndrcvinfo, &sctp_flags);
    if(n > 0) {
        buffer_write_adv(b, n);
        int st = parse_input(b, &management->parser, &error);
        if(request_is_done(st, 0)) {
            ret = pass_process(key);
        }
    } else {
        ret = ERROR;
    }

    return error ? ERROR : ret;
}


static unsigned
pass_confirm(struct selector_key *key) {
    struct management *management = ATTACHMENT(key);

    unsigned  ret     = CONFIRM_PASS;
    struct buffer *wb = &management->write_buffer;

    uint8_t *ptr;
    size_t  count;
    ssize_t  n;

    ptr = buffer_read_ptr(wb, &count);
    n = sctp_sendmsg(key->fd, ptr, count, 0, 0, 0, 0, 0, 0, 0 );
    if(n == -1) {
        ret = ERROR;
    } else {
        buffer_read_adv(wb, n);
        if(!buffer_can_read(wb)) {
            if(selector_set_interest_key(key, OP_READ) == SELECTOR_SUCCESS) {
                if(management->status == CMD_SUCCESS){
                    ret = WAITING_COMMAND;
                }
                else if(management->status == AUTH_ERROR){
                    ret = READING_USER;
                } else {
                    ret = ERROR;
                }
            } else {
                ret = ERROR;
            }
        }
    }

    return ret;
}

///////      COMMAND EXECUTION      ///////

static void command_init(const unsigned state, struct selector_key *key){
    struct management * management  = ATTACHMENT(key);
    parser_init(&management->parser);
}

size_t write_response_with_args(buffer* b, uint8_t status, char* data, size_t data_len){
    uint8_t *ptr;
    size_t  count;
    ptr = buffer_write_ptr(b, &count);
    if(count < data_len + 2)
        return 0;

    buffer_write(b, status);
    buffer_write(b, (uint8_t)data_len);
    memcpy(ptr + 2, data, data_len);
    buffer_write_adv(b, data_len + 2);

    return data_len + 2;
}

static unsigned get_concurrent_connections(struct buffer* b, enum response_status *status){
    int data = metrics->concurrent_connections;

    int digits = 0;
    long aux = data;

    while(aux!=0){
        aux/=10;
        digits++;
    }
    char serialized_data[digits + 1];
    sprintf(serialized_data, "%d",data);

    *status = CMD_SUCCESS;
    if(write_response_with_args(b, 0x00, serialized_data, strlen(serialized_data)) == 0){
        return ERROR;
    }else{
        return RESPOND_COMMAND;
    }

}

static unsigned process_command(struct selector_key* key){
    struct management* management = ATTACHMENT(key);
    struct request_structure * request = &management->parser.request;
    unsigned ret;

    if(request->cmd <= PASS){
        management->status = INVALID_CMD;
        if(write_response_no_args(&management->write_buffer, 0x01) == -1)
            return ERROR;
    }

    switch(request->cmd){
        case CONCURR_CONNECTIONS:
            ret = get_concurrent_connections(&management->write_buffer, &management->status);
            break;
        default:
            management->status = INVALID_CMD;
            ret = ERROR;
    }

    if(ret == ERROR){
        management->status = CMD_ERROR;
    }

    if(selector_set_interest_key(key, OP_WRITE) != SELECTOR_SUCCESS){
        return ERROR;
    }

    return ret;
}

static unsigned command_read(struct selector_key *key){
    struct management * management = ATTACHMENT(key);

    buffer *b = &management->read_buffer;
    unsigned  ret     = READING_CMD;
    bool error = false;
    uint8_t *ptr;
    size_t  count;
    ssize_t  n;

    ptr = buffer_write_ptr(b, &count);
    n = sctp_recvmsg(key->fd, ptr, count, (struct sockaddr *) NULL, 0, &sndrcvinfo, &sctp_flags);
    if(n > 0) {
        buffer_write_adv(b, n);
        int st = parse_input(b, &management->parser, &error);
        if(request_is_done(st, 0)) {
            ret = process_command(key);
        }
    } else {
        ret = ERROR;
    }
    return error ? ERROR : ret;

}

static unsigned
command_respond(struct selector_key *key){
    struct management* management = ATTACHMENT(key);

    unsigned ret = RESPOND_COMMAND;
    struct buffer* write_buffer = &management->write_buffer;

    uint8_t *ptr;
    size_t  count;
    ssize_t  n;

    ptr = buffer_read_ptr(write_buffer, &count);
    n = sctp_sendmsg(key->fd, ptr, count, 0, 0, 0, 0, 0, 0, 0);
    if(n == -1) {
        ret = ERROR;
    } else {
        buffer_read_adv(write_buffer, n);
        if(!buffer_can_read(write_buffer)) {
            if(SELECTOR_SUCCESS == selector_set_interest_key(key, OP_READ)) {
                ret = WAITING_COMMAND;
            } else {
                ret = ERROR;
            }
        }
    }
    return ret;
}
/** definición de handlers para cada estado */
static const struct state_definition client_statbl[] = {
    {
        .state            = READING_USER, 
        .on_arrival       = user_read_init,
        .on_read_ready    = user_read,
    }, {
        .state            = WRITING_USER,
        .on_write_ready   = user_confirm,
    }, {
        .state            = READING_PASS,
        .on_arrival       = pass_read_init,
        .on_read_ready    = pass_read,
    },{
        .state            = CONFIRM_PASS,
        .on_write_ready   = pass_confirm,
    },{
        .state            = WAITING_COMMAND,
        .on_arrival       = command_init,
        .on_read_ready    = command_read,
    },{
        .state            = RESPOND_COMMAND,
        .on_write_ready    = command_respond,
    },{
        .state            = DONE,
    },{
        .state            = ERROR,
    }
};

static const struct state_definition *management_describe_states(void) {
    return client_statbl;
}