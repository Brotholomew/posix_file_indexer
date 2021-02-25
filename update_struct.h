#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>

#include "file.h"

#ifndef update_struct
#define update_struct

typedef struct update {
    pthread_mutex_t * update_mtx;
    pthread_mutex_t * flag_mtx;
    pthread_mutex_t * state_mtx;
    pthread_mutex_t * array_mtx;
    pthread_mutex_t * time_mtx;
    __time_t * last_time;
    file_node ** ARRAY;
    int * current_size;
    int * flag;
    int * t;
    int * state;
    char * dir;
    char * index_file;
} update;

#endif