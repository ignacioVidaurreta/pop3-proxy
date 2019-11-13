#ifndef TRANSFORMATIONS__H
#define TRANSFORMATIONS__H

#include "pop3nio.h"

#define TRANSFORMATION_START_ERR_MSG "failed to start external process:"
#define EXIT_MSG "exiting"

/**
 * creates the tranformation process assigning its communication pipes in
 * the external_to_main_fds and main_to_external_fds fields of the state.
 * It also assigns the pid of the new process to the external_process_pid field.
 */
void create_process(struct state_manager* state);

/**
 * writes what is in the given buffer to the output-to-filter file descriptor.
 */
int write_buffer_to_filter(struct selector_key *key, buffer* buff);

/**
 * reads from the input-from-filter file descriptor and puts the results into buffer.
 */
int read_transformation(struct selector_key *key, buffer* buff);

/**
 * if the state has changed from FILTER kill the external process and free the used
 * resources.
*/
void update_external_process_data(struct state_manager* state);

#endif
