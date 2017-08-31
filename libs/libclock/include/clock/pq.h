/*
 * Prioirty Queue
 * Glenn McGuire & Cameron Lonsdale
 */

#ifndef _PQ_H_
#define _PQ_H_

#include <stdlib.h>
#include <clock/clock.h>

#define PQ_STARTING_SIZE 20

/*
 * Invidivual node in the queue
 */
typedef struct {
    uint64_t priority; /* The priority of the event */
    uint64_t delay; /* The delay of the event */
    timer_callback_t callback; /* The callback function pointer */
    void *data; /* The data to provide the cb */
    uint32_t uid; /* Unique identifier */
    uint8_t repeat; /* Is this a repeating event? */
} event;

/*
 * Priority Queue heap structure
 */
typedef struct {
    event *events; /* Ptr to an array of events */
    uint32_t len;  /* The size of the pq being used */
    uint32_t size; /* The size of the actual pq */
    uint32_t counter; /* Unique id counter */
} priority_queue;

/* 
 * Initialis the priority queue 
 * @returns pointer to priority queue
 */
priority_queue *init_pq();

/* push a value onto the pq 
 * returns a unique id on success, and 0 on failure  
 */
int pq_push(priority_queue *pq, uint64_t priority, uint64_t delay, timer_callback_t cb, void *data, uint8_t repeat, uint32_t uid);

/* pop the front of the pq off */
event *pq_pop(priority_queue *pq);

/* remove an event by its unique id */
int pq_remove(priority_queue *pq, uint32_t id);

/* purge the entire pq */
void pq_purge(priority_queue *pq);

/* check if the pq is empty or not
 * return 1 on empty, 0 othwerise */
int pq_is_empty(priority_queue *pq);

/* get the timestamp value of the current head of the pq */
uint64_t pq_time_peek(priority_queue *pq);

/* get next available id and increment struct global counter */
uint32_t pq_get_next_id(priority_queue *pq);

#endif /* _PQ_H_ */
