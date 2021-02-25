#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "globals.h"

void message(char * msg) {
    fprintf(stderr, "WARNING: %s\n", msg);
}

void set_mutable(int val, int * var, pthread_mutex_t * mutex) {
    if(pthread_mutex_lock(mutex)) ERR("pthread_mutex_lock");
        *var = val;
    if(pthread_mutex_unlock(mutex)) ERR("pthread_mutex_unlock");
}

void get_mutable(int * val, int * var, pthread_mutex_t * mutex) {
    if(pthread_mutex_lock(mutex)) ERR("pthread_mutex_lock");
        *val = *var;
    if(pthread_mutex_unlock(mutex)) ERR("pthread_mutex_unlock");
}

void set_time(long val, long * var, pthread_mutex_t * mutex) {
    if(pthread_mutex_lock(mutex)) ERR("pthread_mutex_lock");
        *var = val;
    if(pthread_mutex_unlock(mutex)) ERR("pthread_mutex_unlock");
}

void get_time(long * val, long * var, pthread_mutex_t * mutex) {
    if(pthread_mutex_lock(mutex)) ERR("pthread_mutex_lock");
        *val = *var;
    if(pthread_mutex_unlock(mutex)) ERR("pthread_mutex_unlock");
}