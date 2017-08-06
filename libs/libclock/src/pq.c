#include <stdlib.h>
#include <clock/pq.h>

void remove_element(priority_queue *pq, int id);

/* Initialised the priority queue */
priority_queue *
init_pq()
{
    return (priority_queue *)calloc(1, sizeof(priority_queue));
}

/*
 * Push an event onto the PQ
 * @param priority_queue *pq, the priority queue
 * @param uint64_t priority, 64 bit integer for priority
 * @param timer_callback_t cb, the call back to run for the event
 * @param void *data, data for the callback function
 * 
 * @returns -1 on error, else a positive uid
 */
int
pq_push(priority_queue *pq, uint64_t priority, timer_callback_t cb, void *data)
{
    if (pq == NULL)
        return -1;

    if (pq->len + 1 >= pq->size) {
        pq->size = pq->size ? pq->size * 2 : PQ_STARTING_SIZE;
        pq->events = (event *)realloc(pq->events, pq->size * sizeof(event));
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

/* 
 * Pop the front of the pq off
 * @param priority_queue *pq
 * @returns pointer to malloc'd event
 *
 * @note You must free() the event when finished
 */
event *pq_pop(priority_queue *pq)
{
    /* sanity checks */
    if (pq == NULL)
        return NULL;

    if (!pq->len)
        return NULL;

    event *front_event = malloc(sizeof(event)); 
    *front_event = pq->events[1];
    remove_element(pq, 1);
    return front_event;
}

/* 
 * Private function to remove an element from the pq given its position in
 * the data structure linearly
 * @param priority_queue *pq
 * @param int id, linear position of the element
 * 
 */
void
remove_element(priority_queue *pq, int id)
{
    int i, j, k;
    pq->events[id] = pq->events[pq->len];
    pq->len--;
    i = id;

    while (1) {
        k = i;
        j = 2 * i;
        if (j <= pq->len && pq->events[j].priority < pq->events[k].priority)
            k = j;

        if (j + 1 <= pq->len && pq->events[j + 1].priority < pq->events[k].priority)
            k = j + 1;

        if (k == i)
            break;

        pq->events[i] = pq->events[k];
        i = k;
    }
    pq->events[i] = pq->events[pq->len + 1];
}

/* 
 * Remove an event by its unique id 
 * @param priority_queue *pq
 * @param int id
 *
 * @returns 1 if element was removed, else 0
 */
int
pq_remove(priority_queue *pq, uint32_t id)
{
    for (unsigned int i = 1; i <= pq->len; i++) {
        if (pq->events[i].uid == id) {
            remove_element(pq, i);
            return 1;           
        }
    }
    return 0;
}

/* 
 * Remove all elements from the queue 
 * @param priority_queue *pq
 */
void
pq_purge(priority_queue *pq)
{
    for (unsigned int i = 1; i <= pq->len; i++)
        remove_element(pq, i);
}

/* 
 * Returns the timestamp value of the head event of the pq
 * @param priority_queue *pq
 * @returns uint64_t timestamp
 */
uint64_t
pq_time_peek(priority_queue *pq)
{
    return pq->events[1].priority;
}

/*
 * Check if the pq is empty
 * @param priority_queue *pq
 * @returns 1 if empty, else 0
 */
int
pq_is_empty(priority_queue *pq)
{
    return pq->len ? 0 : 1;
}
