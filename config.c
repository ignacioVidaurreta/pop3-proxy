#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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