#include <memory.h>
#include <stdlib.h>

#include "include/management_cmd_parser.h"
#include "include/buffer.h"

bool arguments_left_to_read(struct request_parser * parser){
    return parser->nargs_i < parser->nargs;
}

bool finished_parsing(struct request_parser * parser){
    return parser->arg_size_i >= parser->arg_size;
}
enum parser_state parse_argument(struct request_parser * parser, uint8_t arg){
    if(parser->nargs_i == 0)
        parser->request.arg0[parser->arg_size_i++] = arg;
    else if (parser->nargs_i == 1)
        parser->request.arg1[parser->arg_size_i++] = arg;

    if(finished_parsing(parser)){
        if(parser->nargs_i == 0)
            parser->request.arg0[parser->arg_size_i] = '\0';
        else if(parser->nargs_i == 1)
            parser->request.arg1[parser->arg_size_i] = '\0';

        parser->nargs_i++;
        if(arguments_left_to_read(parser))
            return READING_ARG_SIZE;

        return READING_DONE;
    }

    return READING_ARGS;
}
void parser_init(struct request_parser * parser){
    parser -> state = READING_CMD;
    memset(&parser->request, 0, sizeof(parser->request));
}

void set_argument_size(struct request_parser * parser ,uint8_t size){
    parser->arg_size_i = 0; //TODO: Write a better variable name
    parser->arg_size = size;
}
enum parser_state parse_argument_size(struct request_parser * parser, uint8_t c){
    set_argument_size(parser,c);
    if(parser->nargs_i == 0){
        parser->request.arg0 = malloc(c+1);
        parser->request.arg0_size = c;
    }else if(parser->nargs_i == 1){
        parser->request.arg1 = malloc(c+1);
        parser->request.arg1_size = c;
    }

    return READING_ARGS;
}

bool cmd_has_no_arguments(struct request_parser * parser){
    return parser->nargs_i >= parser->nargs;
}

void set_arg_number(struct request_parser * parser, uint8_t num_args){
    parser->nargs_i = 0; // TODO: Write a better variable name
    parser->nargs = num_args;

}

enum parser_state parse_arguments_number(struct request_parser * parser, uint8_t c){
    set_arg_number(parser, c);
    if(cmd_has_no_arguments(parser)){
        return READING_DONE;
    }

    return READING_ARG_SIZE;
}

enum parser_state parse_cmd(struct request_parser * parser, const uint8_t c){
    if( c <= (uint8_t)QUIT ){
        parser->request.cmd = c;
        return READING_ARGS_NUM;
    }

    return INVALID_COMMAND;
}
enum parser_state parser_feed(struct request_parser * parser, const uint8_t c){
    enum parser_state next_state;

    switch(parser->state){
        case READING_CMD:
            next_state = parse_cmd(parser, c);
            break;
        case READING_ARGS_NUM:
            next_state = parse_arguments_number(parser, c);
            break;
        case READING_ARG_SIZE:
            next_state = parse_argument_size(parser, c);
            break;
        case READING_ARGS:
            next_state = parse_argument(parser, c);
            break;
        case READING_DONE:
        case INVALID_COMMAND:
        case INVALID_ARGUMENTS:
        case READING_ERROR:
            next_state = parser->state;
            break;
        default:
            next_state = READING_ERROR;
    }

    parser->state = next_state;
    return next_state;
}

enum parser_state parse_input(buffer* buffer, struct request_parser* parser, bool* error_flag){
    enum parser_state state = parser->state;

    while(buffer_can_read(buffer) && state < READING_DONE){
        const uint8_t c = buffer_read(buffer);
        state = parser_feed(parser, c);
    }
    return state;
}

int write_response_no_args(buffer *b, uint8_t status){
    size_t n;
    uint8_t *buff = buffer_write_ptr(b, &n);
    if(n < 1){
        return -1;
    }
    buff[0] = status;
    buffer_write_adv(b, 1);
    return 1;
}

void request_close(struct request_parser *parser) {
    if(parser->request.arg0 != NULL)
        free(parser->request.arg0);
    if(parser->request.arg1 != NULL)
        free(parser->request.arg1);
}
