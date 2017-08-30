#include <stdlib.h>
#include <clock/pq.h>

static void remove_element(priority_queue *pq, uint32_t id);

/* 
 * Initialis the priority queue 
 * @returns pointer to priority queue
 */
priority_queue *
init_pq(void)
{
    priority_queue *pq = (priority_queue *)calloc(1, sizeof(priority_queue));
    if (!pq)
        return NULL;
    pq->counter = 1;
    return pq;
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
pq_push(priority_queue *pq, uint64_t priority, uint64_t delay, timer_callback_t cb, void *data, uint8_t repeat, uint32_t uid)
{
    /* Sanity checks */
    if (!pq)
        return -1;

    /* Resize the heap DS if we need more room */
    if (pq->len + 1 >= pq->size) {
        pq->size = pq->size ? pq->size * 2 : PQ_STARTING_SIZE;
        pq->events = (event *)realloc(pq->events, pq->size * sizeof(event));
        if (!pq->events)
            return -1;
    }

    uint32_t i = pq->len + 1;
    uint32_t j = i / 2;

    /* Do some shifting to make room for the new event */
    while (i > 1 && pq->events[j].priority > priority) {
        pq->events[i] = pq->events[j];
        i = j;
        j = j / 2;
    }

    pq->events[i].uid = (uid) ? uid : pq->counter++;
    pq->events[i].priority = priority;
    pq->events[i].delay = delay;
    pq->events[i].callback = cb;
    pq->events[i].data = data;
    pq->events[i].repeat = repeat;
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
event *
pq_pop(priority_queue *pq)
{
    /* Sanity checks */
    if (!pq || !pq->len)
        return NULL;

    event *front_event = malloc(sizeof(event)); 
    if (!front_event)
        return NULL;
    
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
static void
remove_element(priority_queue *pq, uint32_t id)
{
    uint32_t i, j, k;
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
    if (!pq || !pq->len)
        return 0;
    for (uint32_t i = 1; i <= pq->len; i++) {
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
    if (!pq || !pq->len)
        return;
    for (uint32_t i = 1; i <= pq->len; i++)
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
    if (pq && pq->len)
        return pq->events[1].priority;
    return 0;
}

/*
 * Check if the pq is empty
 * @param priority_queue *pq
 * @returns 1 if empty, else 0
 */
int
pq_is_empty(priority_queue *pq)
{
    if (pq)
        return pq->len ? 0 : 1;
    return 1;
}

/*
 * Return the next available id, and increments the pq counter.
 * Used for race-condition handling in register_timer
 * @returns uint32_t event id, 0 on error
 */
uint32_t
pq_get_next_id(priority_queue *pq) {
    if (pq)
        return pq->counter++;
    return 0;
}
