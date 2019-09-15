#ifndef TRANSFORMATIONS__H
#define TRANSFORMATIONS__H

#define TRANSFORMATION_START_ERR_MSG "failed to start external process:"
#define EXIT_MSG "exiting"

/**
 * creates the tranformation process returning its STDIN and STDOUT file descriptors
 */
int* create_process();

/**
 * reads what is in the given buffer to the given file descriptor.
 * when done closes the file descriptor.
 */
void write_buffer(char* buffer, int write_fd);

/**
 * reads from the given file descriptor and puts the results into buffer.
 * when done closes the file descriptor.
 */
void read_transformation(char* buffer, int read_fd);

#endif
