/*
 * Queue - as a linked list
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include <stdint.h>

/* queue node definition */
struct queue_node {
    void * data;
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
int queue_add(queue q, void *item);

/* pop an element off the queue 
 * @parmam q, the queue to use
 * @params item, the address to store the item at the front of the queue
 * @returns 0 on success and 1 on error
 */
int queue_pop(queue q, void * item);
