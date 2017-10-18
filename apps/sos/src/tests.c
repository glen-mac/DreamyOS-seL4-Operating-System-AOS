/*
 * Unit tests for milestones
 * 
 * Cameron Lonsdale & Glenn McGuire
 */

#include "tests.h"

#include <assert.h>
#include <clock/clock.h>
#include <utils/time.h>
#include <vm/frametable.h>

#define verbose 5
#include <sys/debug.h>

void callback1(uint32_t id, void *data); 
void callback2(uint32_t id, void *data);
void callback3(uint32_t id, void *data);
void callback4(uint32_t id, void *data);

/* Timer tests */
void
test_m1(void)
{
    /* 100ms periodic callback to print out timestamp */
    register_repeating_timer(MILLISECONDS(100), callback3, NULL);

    /* 1 Second periodic callback to print out timestamp */
    register_timer(SECONDS(1), callback2, NULL);

    /* Several non repeating timers */
    int id1 = register_timer(SECONDS(1), callback3, NULL); // 2
    assert(id1 != 0);

    int id2 = register_timer(SECONDS(2), callback3, NULL); // 3
    assert(id2 != 0);

    int id3 = register_timer(SECONDS(3), callback3, NULL); // 4
    assert(id3 != 0);

    int id4 = register_timer(SECONDS(3), callback3, NULL); // 5
    assert(id4 != 0);

    int id5 = register_timer(SECONDS(2), callback3, NULL); // 6
    assert(id5 != 0);

    int id6 = register_timer(SECONDS(1), callback3, NULL); // 7
    assert(id6 != 0);

    remove_timer(id2);
    remove_timer(id5);

    register_timer(SECONDS(1), callback4, NULL);
    register_timer(SECONDS(2), callback4, NULL);

    /* Timer demonstration with overflow! Prescalar in driver needs to be set to 1 */
    /* 100ms periodic callback to print out timestamp */
    //register_timer(MILLISECONDS(66*100), callback1, NULL);
}

/* frametable tests */
void
test_m2(void)
{
    /* Test 1: Allocate a frame and test read & write */
    seL4_Word frame_id;
    seL4_Word vaddr;
    frame_id = frame_alloc(&vaddr);
    assert(vaddr);

    /* Test you can touch the page */
    *(seL4_Word *)vaddr = 0x37;
    assert(*(seL4_Word *)vaddr == 0x37);

    /* Testing an invalid id does not crash */
    frame_free(-1);
    /* Testing an id that does not map to a valid frame does not crash */
    frame_free(1);

    frame_free(frame_id);
    dprintf(0, "Test 1 Passed\n");

    /* Test 2: Allocate 10 pages and make sure you can touch them all */
    for (int i = 0; i < 10; i++) {
        /* Allocate a page */
        frame_id = frame_alloc(&vaddr);
        assert(vaddr);

        /* Test you can touch the page  */
        *(seL4_Word *)vaddr = 0x37;
        assert(*(seL4_Word *)vaddr == 0x37);

        printf("Page #%d allocated at %p\n",  i, (void *)vaddr);
        frame_free(frame_id);
    }

    dprintf(0, "Test 2 Passed\n");

    /* Test 3: Allocate then Free */
    frame_id = frame_alloc(&vaddr);
    assert(vaddr);

    /* Test you can touch the page */
    *(seL4_Word *)vaddr = 0x37;
    assert(*(seL4_Word *)vaddr == 0x37);
    frame_free(frame_id);

    dprintf(0, "Test 3 Passed\n");

    /* Test 4 Test that you never run out of memory if you always free frames. */
    for (int i = 0; i < 1000000; i++) {
        /* Allocate a page */
        seL4_Word frame_id = frame_alloc(&vaddr);
        assert(vaddr != 0);

        /* Test you can touch the page  */
        *(seL4_Word *)vaddr = 0x37;
        assert(*(seL4_Word *)vaddr == 0x37);

        /* print every 1000 iterations */
        if (i % 10000 == 0)
            printf("Page #%d allocated at %p\n",  i, (void *)vaddr);

        frame_free(frame_id);
    }

    dprintf(0, "Test 4 Passed\n");

    /* Test 5 Test that watermarking works */
    int frames[65];
    for (int i = 0; i < 65; i++) {
        frames[i] = frame_alloc(&vaddr);
        assert(vaddr != 0);

        /* Test you can touch the page  */
        *(seL4_Word *)vaddr = 0x37;
        assert(*(seL4_Word *)vaddr == 0x37);
    }

    for (int i = 0; i < 65; i++)
        frame_free(frames[i]);

    dprintf(0, "Test 5 Passed\n");

    /* Test 5: Test that you eventually run out of memory gracefully, and doesn't crash */
    while (1) {
        /* Allocate a page */
        seL4_Word frame_id = frame_alloc(&vaddr);
        if (vaddr == (seL4_Word)NULL) {
            printf("Out of memory!\n");
            break;
        }
        /* Pin it to prevent it being pages out */
        frame_table_set_chance(frame_id, PINNED);

        /* Test you can touch the page */
        *(seL4_Word *)vaddr = 0x37;
        assert(*(seL4_Word *)vaddr == 0x37);
    }

    dprintf(0, "Test 6 Passed\n");

    assert(frame_table_init(0, 0, 0, 0) == 1);
    dprintf(0, "Test 7 Passed\n");

    dprintf(0, "All tests pass, you are awesome! :)\n");
}

void callback1(uint32_t id, void *data) {
    dprintf(0, "100ms Callback, id:%d, time: %lld\n", id, time_stamp());
    dprintf(0, "registered callback: %d\n", register_timer(100000, callback1, NULL));
}

void callback2(uint32_t id, void *data) {
    dprintf(0, "1 second Callback, id:%d, time: %lld\n", id, time_stamp());
    dprintf(0, "registered callback: %d\n", register_timer(1000000, callback2, NULL));
}

void callback3(uint32_t id, void *data) {
    dprintf(0, "Non-periodic callback id:%d, time: %lld\n", id, time_stamp());
}

void callback4(uint32_t id, void *data) {
    dprintf(0, "Callback 4 id:%d, time: %lld\n", id, time_stamp());
    dprintf(0, "registered callback: %d\n", register_timer(0, NULL, NULL));
}
