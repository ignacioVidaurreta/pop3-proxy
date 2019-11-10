#ifndef METRICS__H
#define METRICS__H

struct metrics_manager {
    int concurrent_connections;
    int number_of_connections;
    unsigned long transfered_bytes;
};

void init_metrics_manager();
char* get_metrics();
void update_metrics_new_connection();
void update_metrics_end_connection();
void update_metrics_transfered_bytes(int n);
void free_metrics();

#endif