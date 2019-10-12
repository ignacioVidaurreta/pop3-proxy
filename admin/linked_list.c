#include <stdio.h>
#include <stdlib.h>
#include "include/linked_list.h"

list_head* create_list(){
    list_head* list = malloc(sizeof(*list));
    list->first = NULL;
    return list;
}

int add_element(list_head* list, void* value){
    if(list->first == NULL){
        node* aux_node = malloc(sizeof(*aux_node));
        if (aux_node == NULL) return 0; // Memory allocation error
        aux_node->elem = value;
        aux_node->next = NULL;
        list->first = aux_node;
        return 1;
    }

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

void free_list(list_head* list){
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

