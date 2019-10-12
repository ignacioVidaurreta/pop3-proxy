#include <stdio.h>

enum CMD {
    AUTH =0,
    FILTER,
    METRICS
};

struct request {
    CMD command;
    int arg_len;
    //linked list --> args
};