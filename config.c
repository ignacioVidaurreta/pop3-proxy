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

void print_usage(char *cmd_name){
    fprintf(stdout, "Usage: %s -h for help\n", cmd_name);
    fprintf(stdout, "Usage: %s -v to print version\n", cmd_name);
    fprintf(stdout, "Usage: %s \t [-e ERROR_FILE]\n", cmd_name);
    fprintf(stdout, "\t\t [-h ]\n");
    fprintf(stdout, "\t\t [-l POP3_ADDRESS ]\n");
    fprintf(stdout, "\t\t [-L MANAGEMENT_ADDRESS ]\n");
    fprintf(stdout, "\t\t [-m REPLACEMENT_MESSAGE ]\n");
    fprintf(stdout, "\t\t [-M CENSURED_MEDIA_TYPES ]\n");
    fprintf(stdout, "\t\t [-o MANAGEMENT_PORT ]\n");
    fprintf(stdout, "\t\t [-p LOCAL_PORT ]\n");
    fprintf(stdout, "\t\t [-P ORIGIN_PORT ]\n");
    fprintf(stdout, "\t\t [-t CMD ] origin_server\n");
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
/*
 *  Port assignment should depend wheter we are using IPv4 or IPv6
 */
void assign_port(struct sockaddr_in6 *address){
    if( ((struct sockaddr*)address)->sa_family == AF_INET){
        ((struct sockaddr_in*)address)->sin_port = htons(options->local_port);
    }else if(((struct sockaddr*)address)->sa_family == AF_INET6){
        address->sin6_port = htons(options->local_port);
    }
}

void change_port(in_port_t *port, char *port_str){
    long port_num = get_port_number(port_str);
    if (*port == -1 ) exit(1); //TODO(Nachito): Configure chain of returns
    *port = port_num;
}

/*
 *  Specify the file where stderr will be redirected when executing
 *  a filter. 
 *  TODO: Make some validations
 */
void change_error_file(char *filename){
    memset(options->error_file, 0, strlen(options->error_file));
    strcpy(options->error_file, filename);
}

/*
 *  
 */
void replace_string(char *previous, char *new){
    char *tmp = realloc(previous, strlen(new)+1);
    if( tmp == NULL){
        fprintf(stdout, "Memory Error: Couldn't reallocate");
        exit(1);
    }

    strncpy(previous, new, strlen(new) +1 );
}
/**
 *  Parse command line arguments to update configuration.
 */
void update_config(const int argc, char* const *argv){
    int opt;

    while((opt = getopt(argc, argv, "p:P:o:vhe:l:L:m:M:")) != -1){
        switch(opt){
            case 'p':
                change_port(&options->local_port, optarg);
                break;
            case 'P':
                change_port(&options->origin_port, optarg);
                break;
            case 'o':
                change_port(&options->management_port, optarg);
                break;
            case 'e':
                change_error_file(optarg);
                break;
            case 'h':
                print_usage(argv[0]);
                exit(0);
                break;
            case 'v':
                fprintf(stdout, "Version: %s\n", options->version);
                exit(0);
                break;
            case 'l':
                fprintf(stdout, "WIP");
                break;
            case 'L':
                fprintf(stdout, "WIP");
                break;
            case 'm':
                replace_string(options->replacement_message, optarg); //TODO(Nachito): Test this
                break;
            case 'M':
                fprintf(stdout, "WIP");
                break;
        }
    }

    assign_port((struct sockaddr_in*)&options->proxy_address);
    //assign_port((struct sockaddr*)&options->managment_address);
}