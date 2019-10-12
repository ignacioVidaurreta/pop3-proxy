
typedef struct node {
    struct node* next;
    void* elem;
} node;

typedef struct list_head{
    node* first;
} list_head;

list_head* create_list();
int add_element(list_head* list, void* value);
int add_elementR(node* curr_node, void* value);
void free_list(list_head* list);
void free_node(node* node);