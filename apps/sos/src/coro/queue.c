/*
 * Queue - as a linked list
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include "queue.h"
#include <stdlib.h>

int
queue_init(queue * q) {
    queue temp = (queue)malloc(sizeof(struct queue_struct));
    if (temp == NULL) {
        *q = NULL;
        return -1;
    }

    temp->size = 0;
    *q = temp;
    return 0;
}

int
queue_add(queue q, coro c) {
    if (q == NULL)
        return -1;
    qnode temp = (qnode)malloc(sizeof(struct queue_node));
    if (!temp)
        return -1;
    temp->routine = c; 
    temp->next = NULL;
    if(q->front == NULL && q->end == NULL){
        q->front = q->end = temp;
        return 0;
    }
    q->end->next = temp;
    q->end = temp;
    return 0;
}

int
queue_pop(queue q, coro * c) {
    if(q == NULL || q->front == NULL) {
        *c = NULL;
        return -1;
    }
    /* set the return value */
    *c = q->front->routine;
    qnode temp = q->front;
    if(q->front == q->end) {
        q->front = q->end = NULL;
    }
    else {
        q->front = q->front->next;
    }
    free(temp);
    return 0;
}
