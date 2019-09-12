#ifndef CONFIG_H
#define CONFIG_H
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
/**
*  Structure that contains the program configuration 
*/
struct config {

    int local_port;
    int origin_port;
    int management_port;

    // Target file to redirect stderr when executing a filter
    char* error_file;

    /* IP Addresses */
    struct sockaddr_in proxy_address;
    struct sockaddr_in managment_address;

    /*  Message that is displayed when a part of the message is
     *  replaced because of a filter    
    */
    char *replacement_message;

    /*  Command used for external transformations
    */
    char* cmd;
    // Current version of the program
    char *version;


};

extern struct config* options;

void initialize_config();

#endif /* CONFIG_H */ 