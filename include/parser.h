#ifndef PARSER_H
#define PARSER_H

void read_command(int fd, char* buffer, int* is_single_line);
void read_multiline_command(char* buffer);
#endif /*PARSER_H*/