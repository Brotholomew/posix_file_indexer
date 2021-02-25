#include "stack.h"
#include <stdlib.h>

void push_new(elem ** head) {
    elem * p_new = (elem *) malloc(sizeof(elem)); 
    p_new->thread_id = 0;
    p_new->next = *head;

    *head = p_new;
}

void pop(elem ** head) {
    if (*head == NULL) return;

    elem * ret = *head;
    *head = (*head)->next;

    free(ret->data);
    free(ret);
}