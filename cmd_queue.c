#include <stdio.h>
#include <stdlib.h>
#include "include/cmd_queue.h"

queue* create_queue(){
    queue* list = malloc(sizeof(*list));
    list->first = NULL;
    list->size = 0;
    return list;
}

int add_element(queue* list, void* value){
    if(list->first == NULL){
        node* aux_node = malloc(sizeof(*aux_node));
        if (aux_node == NULL) return 0; // Memory allocation error
        aux_node->elem = value;
        aux_node->next = NULL;
        list->first = aux_node;
        list->size++;
        return 1;
    }

    list->size++;
    return add_elementR(list->first, value);
    
}

int add_elementR(node* curr_node, void* value){
    if(curr_node->next != NULL){
        return add_elementR(curr_node->next, value);
    }
    node* aux_node = malloc(sizeof(*curr_node));
    if(aux_node == NULL) return 0; //Memory allocation error
    aux_node->elem = value;
    aux_node->next = NULL;
    curr_node->next = aux_node;
    return 1;
}

void* peek(queue* list){
    if(list->first != NULL)
        return list->first->elem;
    
    return NULL;
}

void* pop(queue* list){
    if(list->size == 0) return NULL;

    void* ret = list->first->elem;
    list->first = list->first->next;
    list->size--;
    return ret;
}

void free_queue(queue* list){
    free_node(list->first);
    free(list);
}

void free_node(node* node){
    if(node->next == NULL){
        free(node);
        return;
    }
    free_node(node->next);
    free(node);
}

