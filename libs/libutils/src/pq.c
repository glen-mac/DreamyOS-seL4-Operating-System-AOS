#include <stdlib.h>
#include <utils/pq.h>

/* init the priority queue */
priority_queue *init_pq() {
    return (priority_queue *)calloc(1, sizeof (priority_queue));
}

/* push an value onto the PQ */
void pq_push (priority_queue *h, int priority) {
    if (h->len + 1 >= h->size) {
        h->size = h->size ? h->size * 2 : PQ_STARTING_SIZE;
        h->nodes = (node_t *)realloc(h->nodes, h->size * sizeof (node_t));
    }
    int i = h->len + 1;
    int j = i / 2;
    while (i > 1 && h->nodes[j].priority > priority) {
        h->nodes[i] = h->nodes[j];
        i = j;
        j = j / 2;
    }
    h->nodes[i].priority = priority;
    h->len++;
}

/* pop the front of the pq off */
int pq_pop (priority_queue *h) {
    int i, j, k;
    if (!h->len) {
        return -1;
    }
    int priority = h->nodes[1].priority;
    h->nodes[1] = h->nodes[h->len];
    h->len--;
    i = 1;
    while (1) {
        k = i;
        j = 2 * i;
        if (j <= h->len && h->nodes[j].priority < h->nodes[k].priority) {
            k = j;
        }
        if (j + 1 <= h->len && h->nodes[j + 1].priority < h->nodes[k].priority) {
            k = j + 1;
        }
        if (k == i) {
            break;
        }
        h->nodes[i] = h->nodes[k];
        i = k;
    }
    h->nodes[i] = h->nodes[h->len + 1];
    return priority;
}

