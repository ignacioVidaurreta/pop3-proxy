#ifndef POP3NIO__H
#define POP3NIO__H

#include <sys/socket.h> 
#include "stm.h"
#include "buffer.h"
#include "cmd_queue.h"

struct response_st {
    buffer                  *wb, *rb;
    int                     *fd;
};

enum request_cmd_type {
            RETR,
            LIST,
            CAPA,
            TOP,
            UIDL,
            USER,
            PASS,
            DELE,
            QUIT,
            DEFAULT,
};
struct request_st {
    buffer                      *buffer, *cmd_buffer, *aux_buffer;
    int                         *fd;
    enum request_cmd_type       cmd_type;
    struct next_request         *next_cmd_type;
    struct next_request         *last_cmd_type;
};

struct ehlo_st {
    /** buffer utilizado para I/O */
    buffer               *rb, *wb;
};

struct response_recv {
    buffer                  *response_buffer, *status_buffer;
};


struct next_request {
    enum request_cmd_type cmd_type;
    struct next_request   *next;
};

struct filter_st {
    buffer                      *original_mail_buffer, *filtered_mail_buffer;
    struct parser               *multi_parser;
};
/** maquina de estados general */
enum pop3_state {
  /**
     * Resuelve la direccion del servidor de origen
     *
     * Transiciones:
     *   - CONNECTING     mientras el mensaje no esté completo
     *   - ERROR    ante cualquier error de conexion
     */
    RESOLVE,

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
    CAPA_STATE,


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
     *     - RESPONSE        mientras la respuesta no este completa
     *     - REQUEST         una vez finalizado el envio del mensaje
     *     - ERROR           ante cualquier error (IO/parseo)
     */
    RESPONSE,

    /**
     * envia el contenido del mail recibido al filtro para ser procesado
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

    char*                         error;

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
    bool                          has_pipelining;

    /** maquinas de estados */
    struct state_machine          stm;

    /** estados para el client_fd */
    union {
        struct request_st         request;
        // struct error_st           error;
    } client;

    queue * requests;

    /** estados para el origin_fd */
    union {
        struct request_st         conn;
        struct response_st        response;
        struct ehlo_st            ehlo;
    } origin;

    /** estados para el filter_fd */
    union {
        struct filter_st          filter;
    } filter;

    int                            write_to_filter_fds[2];
    int                            read_from_filter_fds[2];
    bool                           has_filtered_mail;
    int                            external_process;

    /** buffers para ser usados read_buffer, write_buffer.*/
    uint8_t raw_buff_a[BUFFER_MAX_SIZE], raw_buff_b[BUFFER_MAX_SIZE], raw_buff_c[BUFFER_MAX_SIZE], raw_buff_d[BUFFER_MAX_SIZE]; //Si necesitamos más buffers, los podemos agregar aca.
    buffer read_buffer, write_buffer, request_buffer, cmd_request_buffer, request_aux_buffer;
    
    /** cantidad de referencias a este objeto. si es uno se debe destruir */
    unsigned references;

    /** siguiente en el pool */
    struct pop3 *next;
};


void pop3filter_passive_accept();
void pop3_destroy(struct pop3 *state);
void assign_cmd(struct selector_key *key, char *cmd, int cmds_read);
void free_resources();






#endif /* pop3NIO__H */