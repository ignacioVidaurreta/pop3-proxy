#ifndef REQUEST__H
#define REQUEST__H

#include <netinet/in.h>
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

#endif