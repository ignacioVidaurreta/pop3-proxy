#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <getopt.h>
#include <netinet/in.h>

#include "include/config.h"
#include "include/pop3.h"

struct config* options; 

/**
 *  Initialize configuration structure with default values
 */
void initialize_config(){
    options = malloc(sizeof(*options)); //TODO: free in right place

    options->local_port      = 1110;
    options->origin_port     = 110;
    options->management_port = 9090;
    
    options->replacement_message = (char *) calloc(250, sizeof(char));
    strcpy(options->replacement_message, "Parte reemplaza");

    options->error_file       = "/dev/null";    
    options->version          = "1.0";
    options->parse_completely = FALSE;

    memset(&(options->proxy_address), 0, sizeof(options->proxy_address));
    memset(&(options->management_address), 0, sizeof(options->management_address));

    ((struct sockaddr_in*) &options->proxy_address)->sin_family      = AF_INET;
    ((struct sockaddr_in*) &options->proxy_address)->sin_addr.s_addr = htonl(INADDR_ANY);
    ((struct sockaddr_in*) &options->proxy_address)->sin_port        = htons(options->local_port);

    ((struct sockaddr_in*) &options->management_address)->sin_family      = AF_INET;
	((struct sockaddr_in*) &options->management_address)->sin_addr.s_addr = htonl(INADDR_ANY);
    ((struct sockaddr_in*) &options->management_address)->sin_port        = htons(options->local_port);

    options->cmd = malloc(CAT_SIZE*sizeof(char));
    memcpy(options->cmd, "cat", CAT_SIZE);

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
    fprintf(stdout, "\t\t [-c (TRUE: Enables complete Parse / False: Disables complete parse)] \n");
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
in_port_t get_port_number(char* port){

    char *end = 0;
    const long sl = strtol(port, &end, 10);
    
    if (end == port|| '\0' != *end
          || ((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno)
          || sl < 0 || sl > USHRT_MAX) {
           fprintf(stderr, "port should be an integer: %s\n", port);
           return 0;
        }
    return (in_port_t)sl;
    

}

void change_port(in_port_t *port, char *port_str){
    in_port_t port_num = get_port_number(port_str);
    if (*port == 0 ) exit(1); // TODO(Nachito): Configure chain of returns
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
 * sets the command for transforming emails.
 * fails if null.
 */
void setCommand(char *cmd, char *newCmd)
{
    if (newCmd == NULL)
    {
        fprintf(stdout, "Null transformation command");
        exit(1);
    }
    cmd = realloc(cmd, strlen(newCmd));
    memcpy(cmd, newCmd, strlen(newCmd));
}

in_port_t* expose_port(struct sockaddr* address){
    if(address->sa_family == AF_INET)
        return &((struct sockaddr_in*) address)->sin_port;
    else if(address->sa_family == AF_INET6)
        return &((struct sockaddr_in6*) address)->sin6_port;
    else
        //TODO Error handling
        return 0;
}


void set_origin_server(char* address){
    options->origin_server = address;
}
/**
 *  Parse command line arguments to update configuration.
 */
void update_config(const int argc, char* const* argv){
    int opt;

    while((opt = getopt(argc, argv, "p:P:o:c:vhe:l:L:m:M:t")) != -1){
        switch(opt){
            case 'p':
                change_port(&options->local_port, optarg);
                in_port_t * port = expose_port((struct sockaddr*)&options->proxy_address);
                *port = htons(options->local_port);
                break;
            case 'P':
                change_port(&options->origin_port, optarg);
                break;
            case 'o':
                change_port(&options->management_port, optarg);
                break;
            case 'c':
                if(strcmp(optarg, "True") == 0)
                    options->parse_completely = TRUE;
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
            case 't':
                setCommand(options->cmd, optarg);
                break;
        }
    }

    /* TODO(NACHITO): Que esto funque
    if (argv[optind] == NULL) {
        print_usage(argv[0]);
        exit(0);
    }else
        set_origin_server(argv[optind]);
        */
    set_origin_server("0.0.0.0");
}
