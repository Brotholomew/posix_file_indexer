#include "file.h"

#ifndef priority_queue
#define priority_queue

typedef struct node {
    struct node * next;
    file_node * data;
} node;

void Insert(node ** head, file_node * data);
void DeleteMin(node ** head);
void burn_down(node ** head);

#endif