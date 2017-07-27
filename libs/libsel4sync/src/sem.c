#include <sync/sem.h>
#include <sync/mutex.h>
#include <stdlib.h>

struct sync_sem_ {
    sync_mutex_t mutex;
    sync_mutex_t delay;
    int value;
};

sync_sem_t
sync_create_sem(int value) {
    sync_sem_t sem = (sync_sem_t)malloc(sizeof(struct sync_sem_));
    if (!sem){
        return NULL;
    }
    sem->mutex=sync_create_mutex();
    if (!sem->mutex) {
        free(sem);
        return NULL;
    }
    sem->delay=sync_create_mutex();
    if(!sem->delay) {
        sync_destroy_mutex(sem->mutex);
        free(sem);
        return NULL;
    }
    // Delay starts at 0
    // We are treating the mutexes as binary semaphores (since that's what they really are)
    sync_acquire(sem->delay);
    sem->value=value;
    return sem;
}

void
sync_destroy_sem(sync_sem_t sem) {
    sync_destroy_mutex(sem->mutex);
    sync_destroy_mutex(sem->delay);
    free(sem);
}

void
sync_wait(sync_sem_t sem) {
    sync_acquire(sem->mutex);
    sem->value--;
    if (sem->value<0) {
        sync_release(sem->mutex);
        sync_acquire(sem->delay);
    }
    sync_release(sem->mutex);
}

void
sync_signal(sync_sem_t sem) {
    sync_acquire(sem->mutex);
    sem->value++;
    if (sem->value <=0) {
        sync_release(sem->delay);
    } else {
        sync_release(sem->mutex);
    }
}

int
sync_try_wait(sync_sem_t sem) {
    sync_acquire(sem->mutex);
    if (sem->value <= 0) {
        sync_release(sem->mutex);
        return 0;
    }
    sem->value--;
    sync_release(sem->mutex);
    return 1;
}

int
sync_sem_value(sync_sem_t sem) {
    return sem->value;
}


