#ifndef MANAGEMENT_H
#define MANAGEMENT_H
#include <stdio.h>
#include "buffer.h"
void management_passive_accept(struct selector_key* key);
unsigned user_process(struct selector_key *key);
size_t write_response_with_args(buffer* b, uint8_t status, char* data, size_t data_len);

#endif /* MANAGEMENT_H */