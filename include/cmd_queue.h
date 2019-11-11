#ifndef CMD_QUEUE_H
#define CMD_QUEUE_H
#include <stdint.h>

typedef struct node {
    struct node* next;
    void* elem;
} node;

typedef struct list_head{
    node* first;
    unsigned long size;
} list_head;

list_head* create_queue();
int add_element(list_head* list, void* value);
int add_elementR(node* curr_node, void* value);
void free_queue(list_head* list);
void free_node(node* node);

#endif /* CMD_QUEUE_H */