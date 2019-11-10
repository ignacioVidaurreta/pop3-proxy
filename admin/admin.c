#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/sctp.h>
#include <stdbool.h>
#include <ctype.h>
#include <netdb.h>
#include <limits.h>
#include <errno.h>

#define DEFAULT_PORT 9090
#define DEFAULT_ADDRESS "127.0.0.1"
#define BUFFER 2048


bool resolve_address(char *address, uint16_t port, struct addrinfo ** addrinfo) {

  struct addrinfo addr = {
          .ai_family    = AF_UNSPEC,    /* Allow IPv4 or IPv6 */
          .ai_socktype  = SOCK_STREAM,
          .ai_flags     = AI_PASSIVE,   /* For wildcard IP address */
          .ai_protocol  = 0,            /* Any protocol */
          .ai_canonname = NULL,
          .ai_addr      = NULL,
          .ai_next      = NULL,
  };

  char buff[7];
  snprintf(buff, sizeof(buff), "%hu", port);
  if (getaddrinfo(address, buff, &addr, addrinfo) != 0){
    return false;
  }
  return true;
}

static uint16_t parse_port(const char * port) {
    char *end     = 0;
    const long sl = strtol(port, &end, 10);

    if (end == port || '\0' != *end
        || ((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno)
        || sl < 0 || sl > USHRT_MAX) {
        fprintf(stderr, "Error: Argument port %s should be an integer\n", port);
    exit(1);
  }

  return (uint16_t) sl;
}

void print_usage(){
    printf("Usage: ./admin [OPTIONS]\n");
    printf("Options: \n");
    printf("\t-L custom_address");
    printf(": Specifies the address where the admin service will be listening. 127.0.0.1 by default\n");
    printf("\t-o custom_port");
    printf(": Port where the admin service will be listening. 9090 by default \n");
}

bool is_valid_username(char* username){
    return strcmp(username, "admin\n") == 0;
}

bool is_valid_password(char* pass){
    return strcmp(pass, "admin\n") == 0;
}

int main(int argc, char* const* argv){
    printf("------------- PROXY ADMIN CLIENT ------------- \n\n");
    int conn_sock, in, i, ret, flags;
    int * position;
    struct sockaddr_in servaddr;

    // Things that sctp needs
    struct sctp_status status;
    struct sctp_sndrcvinfo sndrcvinfo;
    struct sctp_event_subscribe events;
    struct sctp_initmsg initmsg;

    char buffer[BUFFER];
    position= 0;

    uint16_t port = DEFAULT_PORT;
    char * address = DEFAULT_ADDRESS;
    char * address_2 = DEFAULT_ADDRESS;
    int opt;
    opterr = 0; //Avoid default error message when falling in ?
    size_t size;

    while((opt = getopt(argc, argv, "L:hxo:")) != -1){
        switch(opt){
            case 'L':
                size=strlen(optarg);
                address = malloc(size);
                memcpy(address, optarg, size);
                break;
            case 'o':
                port = parse_port(optarg);
                break;
            case 'h':
                print_usage();
                exit(1);
            case '?':
                if(optopt == 'L')
                    fprintf(stderr, "Remember to write a custom address to listen\n");
                else if(optopt == 'p')
                    fprintf(stderr, "Remember to write a custom port to listen\n");
                else
                    fprintf(stderr, "Unkown option '-%c'\n", optopt);
                exit(1);
            default:
                print_usage();
                exit(1);
        }
    }

    struct addrinfo *addr;
    if(!resolve_address(address, port, &addr)) {
        perror("Unable to resolve address");
        exit(1);
    }

    printf("Connecting to %s:%d\n\n", address, port);

    conn_sock = socket(addr->ai_family, SOCK_STREAM, IPPROTO_SCTP);

    if(conn_sock == -1)
        exit(0);

    ret = connect(conn_sock, addr->ai_addr, addr->ai_addrlen);

    if(ret < 0 ){
        fprintf(stderr, "Couldn't connect to server. Is the server running?\n");
        exit(0);
    }

    printf("\t\tPlease login\t\t\n");
    int ended = 0;
    while(!ended){
        do{
            printf("Write 'login' to login \n");
            if(fgets(buffer, sizeof(buffer), stdin) == NULL)
                fprintf(stderr, "There was an error when reading the input\n");
                buffer[strcspn(buffer, "\n")] = 0; //Remove last \n
        }while(strcmp(buffer, "login") != 0);
        bool user_logged = false;
        bool reading_username = true;
        bool reading_password = true;
        while(!user_logged){
            int success;
            printf("Enter username: \n");
            while(reading_username){
                memset(buffer, 0, sizeof(buffer));
                if(fgets(buffer, sizeof(buffer), stdin) != NULL){
                    success = handle_user(buffer, conn_sock);
                    if(success)
                        reading_username = false;
                    else
                        printf("Invalid username! Please try again\n");
                }
            }
            printf("Enter password: \n");
            while(reading_password){
                memset(buffer, 0, sizeof(buffer));
                if(fgets(buffer, sizeof(buffer), stdin) != NULL){
                    if(is_valid_password(buffer))
                        reading_password = false;
                    else
                        printf("Invalid password! Please try again\n");
                }
            }
            user_logged = true;
        }
        ended=1;
    }

    free(addr);


}