#ifndef BUFFER_H
#define BUFFER_H

#define BUFFER_MAX_SIZE 512
struct buffer_t {
    long write_pointer;
    char *buffer;
    
    long curr_length;
};

struct buffer_t *init_buffer();
void expand_buffer(struct buffer_t *buff);
void free_buffer(struct buffer_t *buff);





#endif /* BUFFER_H */