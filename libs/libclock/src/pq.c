#include <stdlib.h>
#include <clock/pq.h>

/* remove element from pq given its linear position */
void remove_element(priority_queue *pq, int id);

/* init the priority queue */
priority_queue *init_pq() {
    return (priority_queue *)calloc(1, sizeof (priority_queue));
}

/* push an event onto the PQ */
int pq_push (priority_queue *pq, uint64_t priority, timer_callback_t cb, void *data) {
    if (pq->len + 1 >= pq->size) {
        pq->size = pq->size ? pq->size * 2 : PQ_STARTING_SIZE;
        pq->events = (event *)realloc(pq->events, pq->size * sizeof (event));
    }
    int i = pq->len + 1;
    int j = i / 2;
    while (i > 1 && pq->events[j].priority > priority) {
        pq->events[i] = pq->events[j];
        i = j;
        j = j / 2;
    }
    pq->events[i].uid = pq->counter++;
    pq->events[i].priority = priority;
    pq->events[i].callback = cb;
    pq->events[i].data = data;
    pq->len++;
    return pq->events[i].uid;
}

/* pop the front of the pq off */
event *pq_pop(priority_queue *pq) {
    /* sanity check */
    if (!pq->len) {
        return NULL;
    }
    event *frontEvent = malloc(sizeof(event)); 
    *frontEvent = pq->events[1];
    remove_element(pq, 1);
    return frontEvent;
}

/* internal function to remove an element from the pq given its position in
 * the data structure linearly
 */
void remove_element(priority_queue *pq, int id) {
    int i,j,k;
    pq->events[id] = pq->events[pq->len];
    pq->len--;
    i = id;
    while (1) {
        k = i;
        j = 2 * i;
        if (j <= pq->len && pq->events[j].priority < pq->events[k].priority) {
            k = j;
        }
        if (j + 1 <= pq->len && pq->events[j + 1].priority < pq->events[k].priority) {
            k = j + 1;
        }
        if (k == i) {
            break;
        }
        pq->events[i] = pq->events[k];
        i = k;
    }
    pq->events[i] = pq->events[pq->len + 1];
}

/* remove an event by its unique id */
int pq_remove(priority_queue *pq, uint32_t id) {
    for(int i = 1; i <= pq->len; i++) {
        if (pq->events[i].uid == id) {
            remove_element(pq, i);
            return 1;           
        }
    }
    return 0;
}

/* purge the entire pq */
void pq_purge(priority_queue *pq) {
    for(int i = 1; i <= pq->len; i++) {
        remove_element(pq, i);
    }
}

/* get the timestamp value of the first child of the head event of the pq */
uint64_t pq_time_peek(priority_queue *pq) {
    return pq->events[1].priority;
}

/* check if the pq is empty or not */
int pq_is_empty(priority_queue *pq) {
    return pq->len ? 0 : 1;
}
