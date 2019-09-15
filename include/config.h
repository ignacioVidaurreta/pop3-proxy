#ifndef CONFIG_H
#define CONFIG_H
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
/**
*  Structure that contains the program configuration 
*/
struct config {

    in_port_t local_port;
    in_port_t origin_port;
    in_port_t management_port;

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

    /*
     *  This variable defines if when parsing an email, we should parse it in
     *  small chunks (parse_completely = FALSE) or save everything in a buffer
     *  and then parse it (parse_completely = TRUE). This depends on which 
     *  transformation function you are using.
     */
    int parse_completely;

};

void initialize_config();
void print_usage(char *cmd_name);
int get_port_number(char* port);
void change_port(in_port_t *port, char *port_str);
void change_error_file(char *filename);
void replace_string(char *previous, char *new);
void update_config(const int argc, char* const *argv);

#endif /* CONFIG_H */ 