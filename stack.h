#include <pthread.h>

#include "update_struct.h"

#ifndef stack
#define stack

typedef struct elem {
    struct elem * next;

    pthread_t thread_id;
    update * data;
    int val;
} elem;

void push_new(elem ** head);
void pop(elem ** head);

#endif