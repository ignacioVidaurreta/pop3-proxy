#include <stdio.h>
#include <stdlib.h>

#include "include/metrics.h"

extern struct metrics_manager *metrics;

void init_metrics_manager() {
    metrics = malloc(sizeof(struct metrics_manager));
    metrics->concurrent_connections = 0;
    metrics->number_of_connections = 0;
    metrics->transfered_bytes = 0;
}

char * get_metrics() {
    char* aux = malloc(256*sizeof(char));
    sprintf(aux,"Current metrics\n > concurrent_connections: %d\n > number_of_connections: %d\n > transfered_bytes: %ld",metrics->concurrent_connections,
    metrics->number_of_connections, metrics->transfered_bytes);
    return aux;
}

void update_metrics_new_connection() {
     metrics->number_of_connections++;
     metrics->concurrent_connections++;
 }

 void update_metrics_end_connection() {
     metrics->concurrent_connections--;
 }

 void update_metrics_transfered_bytes(int n) {
     metrics->transfered_bytes += n;
 }