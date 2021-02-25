#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "file.h"
#include "priority_queue.h"

void Insert(node ** head, file_node * _data) {
    node ** p = head;
    
    node * p_new = (node *) malloc(sizeof(node));
    p_new->data = _data;

    while((*p) != NULL && (*p)->data->filesize < _data->filesize) 
        p = &((*p)->next);

    p_new->next = (*p);
    *p = p_new;
}

void DeleteMin(node ** head) {
    if (head == NULL) return;

    node * ret = *head;
    *head = (*head)->next;

    free(ret->data);
    free(ret);
}

void burn_down(node ** head) {
    while(*head != NULL){
        node * temp = *head;
        *head = (*head)->next;
        free(temp);
    }
}