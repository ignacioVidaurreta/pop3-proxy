/**
 * buffer.c - buffer con acceso directo (útil para I/O) que mantiene
 *            mantiene puntero de lectura y de escritura.
 */
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "include/buffer.h"

void
buffer_compact(buffer *b) {
    if(b->data == b->read) {
        // nada por hacer
    } else if(b->read == b->write) {
        b->read  = b->data;
        b->write = b->data;
    } else {
        const size_t n = b->write - b->read;
        memmove(b->data, b->read, n);
        b->read  = b->data;
        b->write = b->data + n;
    }
}
   

void bufferReset(buffer *b){
    b->read  = b->data;
    b->write = b->data;
}

void bufferInit(buffer *b, const size_t n, uint8_t *data){
    b->data = data;
    bufferReset(b);
    b->limit = b->data + n;
}

bool bufferCanWrite(buffer *b){
    return b->limit - b->write > 0;
}

uint8_t* bufferWritePointer(buffer *b, size_t *nbyte){
    assert(b->write <= b->limit);
    *nbyte = b->limit - b->write;
    return b->write;
}

bool bufferCanRead(buffer *b){
    return b->write - b->read > 0;
}

uint8_t* bufferReadPointer(buffer *b, size_t *nbytes){
    assert(b->read <= b->write);
    *nbytes = b->write - b->read;
    return b->read;

}

inline void
buffer_write_adv(buffer *b, const ssize_t bytes) {
    if(bytes > -1) {
        b->write += (size_t) bytes;
        assert(b->write <= b->limit);
    }
}

inline void
buffer_read_adv(buffer *b, const ssize_t bytes) {
    if(bytes > -1) {
        b->read += (size_t) bytes;
        assert(b->read <= b->write);

        if(b->read == b->write) {
            // compactacion poco costosa
            buffer_compact(b);
        }
    }
}

