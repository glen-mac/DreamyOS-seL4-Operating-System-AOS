#ifndef _SYNC_SEM_H_
#define _SYNC_SEM_H_

typedef struct sync_sem_* sync_sem_t;

sync_sem_t sync_create_sem(int value);
void sync_destroy_sem(sync_sem_t sem);
void sync_wait(sync_sem_t sem);
void sync_signal(sync_sem_t sem);
int sync_try_wait(sync_sem_t sem);
int sync_sem_value(sync_sem_t sem);

#endif
