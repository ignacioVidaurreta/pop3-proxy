#ifndef PARSER_H
#define PARSER_H

void read_command(int fd, char* buffer);
void read_multiline_command(char* buffer);
void parse_response(char* buffer);
void parse_command(char* buffer);

#endif /*PARSER_H*/