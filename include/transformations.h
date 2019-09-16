#ifndef TRANSFORMATIONS__H
#define TRANSFORMATIONS__H

#include "pop3.h"

#define TRANSFORMATION_START_ERR_MSG "failed to start external process:"
#define EXIT_MSG "exiting"

/**
 * creates the tranformation process assigning its communication pipes in
 * the external_to_main_fds and main_to_external_fds fields of the state.
 * It also assigns the pid of the new process to the external_process_pid field.
 */
void create_process(struct state_manager* state);

/**
 * reads what is in the given buffer to the given file descriptor.
 */
void write_buffer(char* buffer, int write_fd);

/**
 * reads from the given file descriptor and puts the results into buffer.
 */
void read_transformation(char* buffer, int read_fd);

/**
 * if the state has changed from FILTER kill the external process and free the used
 * resources.
*/
void update_external_process_data(struct state_manager* state);

#endif
