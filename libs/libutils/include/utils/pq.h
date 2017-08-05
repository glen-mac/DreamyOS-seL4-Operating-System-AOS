#include <stdlib.h>

#define PQ_STARTING_SIZE 20

/* individual node */
typedef struct {
    int priority;		/* the priority of the node */
} node_t;

/* pq heap structure */
typedef struct {
    node_t *nodes;		/* ptr to an array of pq nodes */
    int len;			/* the size of the pq being used */
    int size;			/* the size of the actual pq */
} priority_queue;

/* init the priority queue */
priority_queue *init_pq();

/* push a value onto the pq */
void pq_push(priority_queue *h, int priority);

/* pop the front of the pq off */
int pq_pop(priority_queue *h);

