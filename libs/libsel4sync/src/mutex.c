#include <sync/mutex.h>
#include <stdlib.h>
#include <assert.h>


#define MUTEX_MAGIC 0x5EED

struct sync_mutex_ {
    void* ep;
    seL4_CPtr mapping;
};

sync_mutex_t
sync_create_mutex() {
    sync_mutex_t mutex;

    mutex = (sync_mutex_t) malloc(sizeof(struct sync_mutex_));
    if (!mutex)
        return NULL;

    mutex->ep = sync_new_ep(&mutex->mapping, MUTEX_MAGIC);
    if(mutex->ep == NULL){
        free(mutex);
        return NULL;
    }

    // Prime the endpoint
    sync_release(mutex);
    return mutex;
}

void
sync_destroy_mutex(sync_mutex_t mutex) {
    sync_free_ep(mutex->ep);
    free(mutex);
}

void
sync_acquire(sync_mutex_t mutex) {
    seL4_Word badge;
    assert(mutex);
    seL4_Wait(mutex->mapping, &badge);
    assert(badge = MUTEX_MAGIC);
}

void
sync_release(sync_mutex_t mutex) {
    // Wake the next guy up
    seL4_Notify(mutex->mapping, 0);
}

int
sync_try_acquire(sync_mutex_t mutex) {
    seL4_Word badge;
    seL4_Poll(mutex->mapping, &badge);
    return badge == MUTEX_MAGIC;
}


