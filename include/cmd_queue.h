#ifndef CMD_QUEUE_H
#define CMD_QUEUE_H
#include <stdint.h>

typedef struct node {
    struct node* next;
    void* elem;
} node;

typedef struct queue{
    node* first;
    unsigned long size;
} queue;

queue* create_queue();
int add_element(queue* list, void* value);
int add_elementR(node* curr_node, void* value);
void* peek(queue* list);
void* pop(queue* list);
void free_queue(queue* list);
void free_node(node* node);

#endif /* CMD_QUEUE_H */