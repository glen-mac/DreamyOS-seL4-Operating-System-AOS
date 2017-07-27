

#include <sync/mutex.h>
#include <sync/sem.h>

#include <stdio.h>
#include <assert.h>

#define test_assert(tst)        \
    do {                        \
        if(!tst){               \
            printf("FAILED\n"); \
            assert(tst);        \
        }                       \
    }while(0)

/* Return 1 if tests fail */
int sync_run_unit_test(void);

/* Test mutex */
static int 
test_mutex(void){
    sync_mutex_t m1, m2;
    printf("UNIT TEST | sync/mutex: ");
    m1 = sync_create_mutex();
    test_assert(m1);
    m2 = sync_create_mutex();
    test_assert(m2);
    /* Test simple operations */
    test_assert(sync_try_acquire(m1));
    test_assert(!sync_try_acquire(m1));
    sync_release(m1);
    test_assert(sync_try_acquire(m1));
    sync_release(m1);
    sync_acquire(m1);
    sync_release(m1);
    sync_acquire(m1);
    sync_release(m1);
    /* Test independance */
    test_assert(sync_try_acquire(m1));
    test_assert(sync_try_acquire(m2));
    sync_release(m1);
    sync_release(m2);
    /* Clean up */
    sync_destroy_mutex(m1);
    sync_destroy_mutex(m2);
    printf("PASSED\n");
    return 0;
}

/* Test semaphores */
static int
test_sem(void){
    sync_sem_t s1, s2;
    printf("UNIT TEST | sync/semiphores: ");
    s1 = sync_create_sem(0);
    test_assert(s1);
    s2 = sync_create_sem(1);
    test_assert(s2);
    /* Test that initial values are correct */
    test_assert(sync_sem_value(s1) == 0);
    test_assert(sync_sem_value(s2) == 1);
    /* Test that we can wait as appropriate */
    test_assert(sync_try_wait(s1) == 0);
    test_assert(sync_try_wait(s2) == 1);
    /* Test that the previous ops did/didn't changed the state */
    test_assert(sync_try_wait(s1) == 0);
    test_assert(sync_try_wait(s2) == 0);
    test_assert(sync_sem_value(s1) == 0);
    test_assert(sync_sem_value(s2) == 0);
    /* Test signal */
    sync_signal(s1);
    sync_signal(s2);
    sync_signal(s2);
    test_assert(sync_sem_value(s1) == 1);
    test_assert(sync_sem_value(s2) == 2);
    /* Clean up */
    sync_destroy_sem(s1);
    sync_destroy_sem(s2);
    printf("PASSED\n");
    return 0;
}

int
sync_run_unit_test(void){
    int err = 0;
    err += test_mutex();
    err += test_sem();
    return err;

}
