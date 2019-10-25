#ifndef PARSER_H
#define PARSER_H
#include "pop3.h"

void read_command(int fd, char* buffer);
void read_multiline_command(char* buffer, int start, int end,struct state_manager* state);
void parse_response(uint8_t* buffer, struct state_manager* state);
void transform_response(char* buffer, struct state_manager* state);
void parse_command(char* buffer, struct state_manager* state);

#endif /*PARSER_H*/
