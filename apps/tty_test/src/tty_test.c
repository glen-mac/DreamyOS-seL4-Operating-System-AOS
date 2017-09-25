/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/****************************************************************************
 *
 *      $Id:  $
 *
 *      Description: Simple milestone 0 test.
 *
 *      Author:         Godfrey van der Linden
 *      Original Author:    Ben Leslie
 *
 ****************************************************************************/

#include <assert.h>
#include <fcntl.h>
#include <sel4/sel4.h>
#include <sos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ttyout.h>
#include <unistd.h>
#include <utils/page.h>

#define NPAGES 27
#define TEST_ADDRESS 0x20000000

#define SMALL_BUF_SZ 2
#define BUF_SZ (2 * PAGE_SIZE_4K) /* 2 page buffer */

static void test_m3(void);
static void test_m4(void);
static void test_m7(void);
static void thread_block(void);

char *file_name = "test_file_name";

int
main(void)
{
    // thread_block();
    printf(">>> tty_test program started <<<\n");
    // test_m3();
    // test_m4();
    // test_m7();
    sleep(5);

    exit(0);
    printf("Should not print");    
}

static void
do_pt_test(char *buf)
{
    /* set */
    for (int i = 0; i < NPAGES; i++) {
        buf[i * PAGE_SIZE_4K] = i;
    }

    /* check */
    for (int i = 0; i < NPAGES; i++) {
        assert(buf[i * PAGE_SIZE_4K] == i);
    }
}

static void
test_m3(void)
{
    /* need a decent sized stack */
    char buf1[NPAGES * PAGE_SIZE_4K];

    /* check the stack is above phys mem */
    assert((void *) buf1 > (void *) TEST_ADDRESS);

    do_pt_test(buf1);

    /* This should generate a Read fault type  */
    seL4_Word *addr = (seL4_Word *)TEST_ADDRESS;

    /* heap test */
    char *buf2 = malloc(NPAGES * PAGE_SIZE_4K);
    assert(buf2);
    do_pt_test(buf2);
    free(buf2);

    printf("M3 Tests passed, Now generating a permissions fault\n");

    /* This should generate a permissions fault, we cant write to the code section */
    addr = (seL4_Word *)0x00008000;
    *addr = 5;
}

static void
test_m4(void)
{
    int result;

    char test_str[] = "Basic test string for read/write\n";
    char small_buf[SMALL_BUF_SZ];

    int console_fd = open("console", O_RDWR);
    assert(console_fd != -1);

    /* test a small string from the code segment */
    result = sos_sys_write(console_fd, test_str, strlen(test_str));
    assert(result == strlen(test_str));

    /* Write from a non resident buffer, should get mapped int */
    char non_resident_buffer[BUF_SZ];
    sos_sys_write(console_fd, non_resident_buffer, BUF_SZ);

    printf("Enter %d bytes\n", SMALL_BUF_SZ);

    /* test reading to a small buffer */
    result = sos_sys_read(console_fd, small_buf, SMALL_BUF_SZ);
    /* make sure you type in at least SMALL_BUF_SZ */
    assert(result == SMALL_BUF_SZ);

    /* test reading into a large on-stack buffer */
    char stack_buf[BUF_SZ];
    /* for this test you'll need to paste a lot of data into 
      the console, without newlines */

    printf("Enter %lu bytes for the stack\n", BUF_SZ);
    result = sos_sys_read(console_fd, stack_buf, BUF_SZ);
    assert(result == BUF_SZ);

    printf("Now printing %lu bytes from the stack\n", BUF_SZ);
    result = sos_sys_write(console_fd, stack_buf, BUF_SZ);
    assert(result == BUF_SZ);

    /* this call to malloc should trigger an brk */
    char *heap_buf = malloc(BUF_SZ);
    assert(heap_buf != NULL);

    /* for this test you'll need to paste a lot of data into
       the console, without newlines */
    printf("Enter %lu bytes for the heap\n", BUF_SZ);
    result = sos_sys_read(console_fd, heap_buf, BUF_SZ);
    assert(result == BUF_SZ);

    printf("Now printing %lu bytes from the heap\n", BUF_SZ);
    result = sos_sys_write(console_fd, heap_buf, BUF_SZ);
    assert(result == BUF_SZ);

    free(heap_buf);

    /* try sleeping */
    for (int i = 0; i < 5; i++) {
       time_t prev_seconds = time(NULL);
       sleep(1);
       time_t next_seconds = time(NULL);
       assert(next_seconds > prev_seconds);
       printf("Tick\n");
    }

    /* Checking permissions */
    printf("printing from a code section, dont be surprised if these look weird\n");
    result = sos_sys_write(console_fd, (char *)0x10000, 3);
    assert(result == 3);

    result = sos_sys_read(console_fd, (char *)0x10000, 3);
    assert(result == -1);

    printf("\nM4 Tests pass\n");
}

static void
test_m7(void)
{
    printf("my id is %d\n", sos_my_id());

    pid_t pid = sos_process_create("sosh");
    printf("pid is %d", pid);

    printf("waiting for exit");

    /* Assert process that exited was one we were waiting for */
    assert(sos_process_wait(pid) == pid);
}

/* 
 * Block a thread forever
 * we do this by making an unimplemented system call.
 */
static void
thread_block(void)
{
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetTag(tag);
    seL4_SetMR(0, 999);
    seL4_Call(SOS_IPC_EP_CAP, tag);
}
