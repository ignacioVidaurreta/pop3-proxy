#ifndef PC_2019B_01_MANAGEMENT_CMD_PARSER_H
#define PC_2019B_01_MANAGEMENT_CMD_PARSER_H
#include <stdio.h>
#include <stdint.h>
#include "buffer.h"


enum request_CMD {
    USER                        = 0x00,
    PASS                        = 0x01,
    CONCURR_CONNECTIONS         = 0x02,
    SET_TRANSFORMATION          = 0x03,
    TRANSFERRED_BYTES           = 0x04,
    QUIT                        = 0x05
};

struct request_structure {
    enum request_CMD cmd;
    uint8_t *arg0;
    uint8_t *arg1;
    size_t arg0_size;
    size_t arg1_size;
};

enum parser_state {
    READING_CMD,
    READING_ARGS_NUM,
    READING_ARG_SIZE,
    READING_ARGS,

    READING_DONE,

    INVALID_COMMAND,
    INVALID_ARGUMENTS,
    READING_ERROR,
};

enum response_status {
    CMD_SUCCESS = 0x00,
    AUTH_ERROR  = 0x01,
    INVALID_CMD = 0x02,
    INVALID_ARG = 0x03,
    CMD_ERROR   = 0x04,
};

// Definición del parser
struct request_parser {
    /** El request que esta siendo parseado*/
    struct request_structure     request;
    /** Qué parte del comando estamos*/
    enum   parser_state         state;
    /** Número de argumentos */
    uint8_t nargs;
    /** Argumentos leídos */
    uint8_t nargs_i;
    /** ultimo narg parseado */
    uint8_t arg_size;
    /** bytes del argumento ya leidos*/
    uint8_t arg_size_i;


};

void parser_init(struct request_parser * parser);
enum parser_state parse_input(buffer* buffer, struct request_parser* parser, bool* error_flag);
enum parser_state parser_feed(struct request_parser * parser, uint8_t c);

/* A method for each parser_state */
enum parser_state parse_cmd(struct request_parser * parser, uint8_t c);

enum parser_state parse_arguments_number(struct request_parser * parser, uint8_t c);
bool cmd_has_no_arguments(struct request_parser * parser);

enum parser_state parse_argument_size(struct request_parser * parser, uint8_t c);
void set_argument_size(struct request_parser * parser, uint8_t c);

enum parser_state parse_argument(struct request_parser * parser, uint8_t arg);

int write_response_no_args(buffer *b, uint8_t status);

#endif //PC_2019B_01_MANAGEMENT_CMD_PARSER_H
