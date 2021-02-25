#include <stdlib.h>
#include <stdio.h>

#ifndef globals
#define globals

#define MAX_PATH_LENGTH 500
#define MAX_FILENAME_LENGTH 50
#define MAX_INPUT_LENGTH 50
#define MAX_HOME_DIR_LENGTH 255

#define ERR(msg) (perror(msg), fprintf(stderr, "Error at line: %s:%d", __FILE__, __LINE__), exit(EXIT_FAILURE))

void message(char * msg);
void set_mutable(int val, int * var, pthread_mutex_t * mutex);
void get_mutable(int * val, int * var, pthread_mutex_t * mutex);

void set_time(long val, long * var, pthread_mutex_t * mutex);
void get_time(long * val, long * var, pthread_mutex_t * mutex);

#endif