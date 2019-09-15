#ifndef METRICS__H
#define METRICS__H

void init_metrics_manager();
char* get_metrics();

struct metrics_manager {
int concurrent_connections;
int number_of_connections;
int transfered_bytes;
};

#endif