/*
 * Queue - as a linked list
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include <sel4/sel4.h>
#include <stdint.h>
#include "picoro.h"

/* queue node definition */
struct queue_node {
    coro routine;
    struct queue_node * next;
};
typedef struct queue_node * qnode;

/* queue definition */
struct queue_struct {
    qnode front;  
    qnode end;  
    uint32_t size;
};
typedef struct queue_struct * queue;

/* create a queue 
 * @parmam[out] q, the address to store the new queue
 * @returns 0 on success and 1 on error
 */
int queue_init(queue * q);

/* add an element to the queue 
 * @parmam q, the queue to use
 * @param item, the address of the item to store
 * @returns 0 on success and 1 on error
 */
int queue_add(queue q, coro c);

/* pop an element off the queue 
 * @parmam q, the queue to use
 * @params item, the address to store the item at the front of the queue
 * @returns 0 on success and 1 on error
 */
int queue_pop(queue q, coro * c);
