#include <stdio.h>
#include "include/linked_list.h"

enum CMD {
    AUTH =0,
    FILTER,
    METRICS
};

struct request {
    CMD command;
    int arg_len;
    list_head* args;
};