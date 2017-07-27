#ifndef _SYNC_MUTEX_H_
#define _SYNC_MUTEX_H_

#include <sel4/sel4.h>

typedef struct sync_mutex_* sync_mutex_t;

sync_mutex_t sync_create_mutex();
void sync_destroy_mutex(sync_mutex_t mutex);
void sync_acquire(sync_mutex_t mutex);
void sync_release(sync_mutex_t mutex);

/* Non blocking attempt to acquire a mutex. Returns 1 if mutex acquired,
   or 0 otherewise */
int sync_try_acquire(sync_mutex_t mutex);

/* These functions should be provided by the application */
extern void* sync_new_ep(seL4_CPtr* ep, int badge);
extern void sync_free_ep(void* ep);

#endif
