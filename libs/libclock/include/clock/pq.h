#include <stdlib.h>
#include <clock/clock.h>

#define PQ_STARTING_SIZE 20

/* individual node */
typedef struct {
    uint64_t priority;		    /* the priority of the event */
    timer_callback_t callback;	/* the callback function pointer */
    void *data;		            /* the data to provide the cb */
    uint32_t uid;               /* unique identifier */
} event;

/* pq heap structure */
typedef struct {
    event *events;		/* ptr to an array of events */
    int len;			/* the size of the pq being used */
    int size;			/* the size of the actual pq */
    int counter;        /* unique id counter */
} priority_queue;

/* init the priority queue */
priority_queue *init_pq();

/* push a value onto the pq 
 * returns a unique id on success, and 0 on failure  
 */
int pq_push(priority_queue *pq, uint64_t priority, timer_callback_t cb, void *data);

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
