#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <getopt.h>

#include "include/config.h"

struct config* options; 

/**
 *  Initialize configuration structure with default values
 */
void initialize_config(){
    options = malloc(sizeof(*options));

    options->local_port      = 1110;
    options->origin_port     = 110;
    options->management_port = 9090;

    options->error_file      = "/dev/null";
    options->replacement_message = (char *) calloc(250, sizeof(char));
    strcpy(options->replacement_message, "Parte reemplaza");
    options->version = "1.0";

    memset(&(options->proxy_address), 0, sizeof(options->proxy_address));
    options->proxy_address.sin_family      = AF_INET;
    options->proxy_address.sin_addr.s_addr = htonl(INADDR_ANY);
    options->proxy_address.sin_port        = htons(options->local_port);

    /* TODO: Too advanced for the current state of the project
        -> Managment Address
        -> media-types-censurables
        -> cmd
    */

}
/*
 *  Gets int value of the string port
 *  Code taken for Juan F. Codagnones' "SocksV5 Sockets Implementation"
 */
int get_port_number(char* port){

    char *end = 0;
    const long sl = strtol(port, &end, 10);
    
    if (end == port|| '\0' != *end
          || ((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno)
          || sl < 0 || sl > USHRT_MAX) {
           fprintf(stderr, "port should be an integer: %s\n", port);
           return -1;
        }
    return sl;
    

}

/**
 *  Parse command line arguments to update configuration.
 */
void update_config(const int argc, char* const *argv){
    int opt;
    long port;

    while((opt = getopt(argc, argv, "p:")) != -1){
        switch(opt){
            case 'p':
                port = get_port_number(optarg);
                if (port == -1 ) exit(1); //TODO(Nachito): Configure chain of returns
                options->local_port = port;
                break;
        }
    }
}