#ifndef REQUEST_H
#define REQUEST_H

#include <stdint.h>
#include <stdbool.h>

#include <netinet/in.h>

#include "buffer.h"

/*   The SOCKS request is formed as follows:
 *
 *      +----+-----+-------+------+----------+----------+
 *      |VER | CMD |  RSV  | ATYP | DST.ADDR | DST.PORT |
 *      +----+-----+-------+------+----------+----------+
 *      | 1  |  1  | X'00' |  1   | Variable |    2     |
 *      +----+-----+-------+------+----------+----------+
 *
 *   Where:
 *
 *        o  VER    protocol version: X'05'
 *        o  CMD
 *           o  CONNECT X'01'
 *           o  BIND X'02'
 *           o  UDP ASSOCIATE X'03'
 *        o  RSV    RESERVED
 *        o  ATYP   address type of following address
 *           o  IP V4 address: X'01'
 *           o  DOMAINNAME: X'03'
 *           o  IP V6 address: X'04'
 *        o  DST.ADDR       desired destination address
 *        o  DST.PORT desired destination port in network octet
 *           order
 */
/*
 * miembros de la sección 4: `Requests'
 *  - Cmd
 *  - AddressType
 *  - Address: IPAddress (4 y 6), DomainNameAdddres
 */

enum socks_req_cmd {
    socks_req_cmd_connect   = 0x01,
    socks_req_cmd_bind      = 0x02,
    socks_req_cmd_associate = 0x03,
};

enum socks_addr_type {
    socks_req_addrtype_ipv4   = 0x01,
    socks_req_addrtype_domain = 0x03,
    socks_req_addrtype_ipv6   = 0x04,
};

union socks_addr {
    char fqdn[0xff];
    struct sockaddr_in  ipv4;
    struct sockaddr_in6 ipv6;
};

struct request {
    enum  socks_req_cmd   cmd;
    enum  socks_addr_type dest_addr_type;
    union socks_addr      dest_addr;
    /** port in network byte order */
    in_port_t             dest_port;
};

enum request_state {
   request_version,
   request_cmd,
   request_rsv,
   request_atyp,
   request_dstaddr_fqdn,
   request_dstaddr,
   request_dstport,

   // apartir de aca están done
   request_done,

   // y apartir de aca son considerado con error
   request_error,
   request_error_unsupported_version,
   request_error_unsupported_atyp,

};

struct request_parser {
   struct request *request;
   enum request_state state;
   /** cuantos bytes tenemos que leer*/
   uint8_t n;
   /** cuantos bytes ya leimos */
   uint8_t i;
};

/*
 * "...
 * 6.  Replies
 *
 * The SOCKS request information is sent by the client as soon as it has
 * established a connection to the SOCKS server, and completed the
 * authentication negotiations.  The server evaluates the request, and
 * returns a reply formed as follows:
 * ..."-- sección 6
 *
 */
enum socksResponseStatus {
    statusSucceeded                          = 0x00,
    statusGeneralSOCKSServerFailure          = 0x01,
    statusConnectionNotAllowedByRuleset      = 0x02,
    statusNetworkUnreachable                 = 0x03,
    statusHostUnreachable                    = 0x04,
    statusConnectionRefused                  = 0x05,
    statusTTLExpired                         = 0x06,
    statusCommandNotSupported                = 0x07,
    statusAddressTypeNotSupported            = 0x08,
};


/** inicializa el parser */
void 
request_parser_init (struct request_parser *p);

/** entrega un byte al parser. retorna true si se llego al final  */
enum request_state 
request_parser_feed (struct request_parser *p, const uint8_t c);

/**
 * por cada elemento del buffer llama a `request_parser_feed' hasta que
 * el parseo se encuentra completo o se requieren mas bytes.
 *
 * @param errored parametro de salida. si es diferente de NULL se deja dicho
 *   si el parsing se debió a una condición de error
 */
enum request_state
request_consume(buffer *b, struct request_parser *p, bool *errored);

/**
 * Permite distinguir a quien usa socks_hello_parser_feed si debe seguir
 * enviando caracters o no. 
 *
 * En caso de haber terminado permite tambien saber si se debe a un error
 */
bool request_is_done(const enum request_state st, bool *errored);

void
request_close(struct request_parser *p);

/**
 * serializa en buff la una respuesta al request.
 *
 * Retorna la cantidad de bytes ocupados del buffer o -1 si no había
 * espacio suficiente.
 */
extern int
request_marshall(buffer *b,
                 const enum socksResponseStatus status);


/** convierte a errno en socks_response_status */
enum socksResponseStatus errno_to_socks(int e);

#include <netdb.h>
#include <arpa/inet.h>

/** se encarga de la resolcuión de un request */
enum socksResponseStatus
cmd_resolve(struct request* request,  struct sockaddr **originaddr,
            socklen_t *originlen, int *domain);

#endif
