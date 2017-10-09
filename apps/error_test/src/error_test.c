/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <assert.h>
#include <limits.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <sys/time.h>
#include <utils/time.h>

#include <sel4/sel4.h>
#include <sos.h>

#include "test_constants.h"

void timer_errors() {
    /* We just shouldn't sleep. */
    int64_t t1 = sos_sys_time_stamp();
    sos_sys_usleep(0);
    sos_sys_usleep(-1);
    sos_sys_usleep(-234324);
    sos_sys_usleep(INT_MIN);
    assert(sos_sys_time_stamp() - t1 < 3000000);
}

void file_errors() {
    char path[2 * MAX_PATH_LENGTH];
    for (int i = 0; i < 2 * MAX_PATH_LENGTH; i++) {
        path[i] = 'a';
    }

    path[2 * MAX_PATH_LENGTH - 1] = '\0';
    int fd = sos_sys_open(path, O_RDONLY);
    assert(fd == -1);

    path[MAX_PATH_LENGTH] = '\0';
    fd = sos_sys_open(path, O_RDONLY);
    assert(fd == -1);

    path[0] = '\0';
    fd = sos_sys_open(path, O_RDONLY);
    assert(fd == -1);

    fd = sos_sys_open(NULL, O_RDONLY);
    assert(fd == -1);

    fd = sos_sys_open((char *)1000, O_RDONLY);
    assert(fd == -1);

    fd = sos_sys_open((char *)~0, O_RDWR);
    assert(fd == -1);

    fd = sos_sys_open("a_new_file.txt", O_RDWR);
    assert(fd != -1);
    assert(sos_sys_close(fd) == 0);

    fd = sos_sys_open("a_new_file.txt", FM_READ | FM_WRITE);
    assert(fd == -1);

    fd = sos_sys_open("a_new_file_2.txt", 890244);
    assert(fd == -1);

    assert(sos_sys_close(-1) == -1);
    assert(sos_sys_close(-243243244) == -1);
    assert(sos_sys_close(1000) == -1);
    assert(sos_sys_close(543543545) == -1);

    char buff[3 * PAGE_SIZE];
    assert(sos_sys_write(-1, buff, 1) == -1);
    assert(sos_sys_write(-34234324, buff, 1000) == -1);
    assert(sos_sys_write(1000, buff, 3 * PAGE_SIZE) == -1);
    assert(sos_sys_write(454355455, buff, 234) == -1);
    assert(sos_sys_write(~0, buff, 234) == -1);
    assert(sos_sys_read(-1, buff, 1) == -1);
    assert(sos_sys_read(-34234324, buff, 1000) == -1);
    assert(sos_sys_read(1000, buff, 3 * PAGE_SIZE) == -1);
    assert(sos_sys_read(454355455, buff, 234) == -1);
    assert(sos_sys_read(~0, buff, 234) == -1);

    fd = sos_sys_open("a_new_file.txt", O_WRONLY);
    assert(fd != -1);
    /* Not really an error but rather a corner-case. */
    assert(sos_sys_write(fd, NULL, 0) == 0);
    assert(sos_sys_write(fd, NULL, 3242) == -1);
    assert(sos_sys_write(fd, (char *)1000, 1) == -1);
    assert(sos_sys_write(fd, (char *)~0, 1000) == -1);
    assert(sos_sys_write(fd, buff, ~0) == -1);
    assert(sos_sys_read(fd, buff, 1) == -1);
    /* Not really an error but rather a corner case. */
    assert(sos_sys_write(fd, buff, 0) == 0);
    assert(sos_sys_write(fd, buff, 1) == 1);
    assert(sos_sys_close(fd) == 0);
    assert(sos_sys_write(fd, buff, 1) == -1);

    fd = sos_sys_open("a_new_file.txt", O_RDONLY);
    assert(fd != -1);
    assert(sos_sys_read(fd, NULL, 0) == 0);
    assert(sos_sys_read(fd, NULL, 3242) == -1);
    assert(sos_sys_read(fd, (char *)1000, 1) == -1);
    assert(sos_sys_read(fd, (char *)~0, 1000) == -1);
    assert(sos_sys_read(fd, buff, ~0) == -1);
    assert(sos_sys_write(fd, buff, 1) == -1);
    /* Not really an error but rather a corner case. */
    assert(sos_sys_read(fd, buff, 0) == 0);
    assert(sos_sys_read(fd, "a_new_file.txt", 1) == -1);
    assert(sos_sys_close(fd) == 0);
    assert(sos_sys_read(fd, buff, 1) == -1);

    fd = sos_sys_open("console", O_RDONLY);
    assert(sos_sys_open("console", O_RDONLY) == -1);
    assert(sos_sys_close(fd) == 0);

    assert(sos_sys_open("swap", O_RDONLY) == -1);

    char name_buff[MAX_PATH_LENGTH];
    assert(sos_getdirent(-1, name_buff, MAX_PATH_LENGTH) == -1);
    assert(sos_getdirent(-34214, name_buff, MAX_PATH_LENGTH) == -1);
    assert(sos_getdirent(342423, name_buff, MAX_PATH_LENGTH) == -1);
    assert(sos_getdirent(0, name_buff, ~0) == -1);
    assert(sos_getdirent(0, "a_new_file.txt", 100) == -1);
    assert(sos_getdirent(0, NULL, 100) == -1);
    assert(sos_getdirent(0, (void *)~0, 1000) == -1);
    assert(sos_getdirent(0, name_buff, 0) == -1);

    sos_stat_t stat;
    assert(sos_stat("non_existant_file.wmv", &stat) == -1);
    assert(sos_stat(NULL, &stat) == -1);
    assert(sos_stat((void *)1000, &stat) == -1);
    assert(sos_stat((void *)~0, &stat) == -1);
    assert(sos_stat("a_new_file.txt", NULL) == -1);
    assert(sos_stat("a_new_file.txt", (void *)~0) == -1);
    assert(sos_stat("a_new_file.txt", (void *)1000) == -1);
    assert(sos_stat("a_new_file.txt", (void *)"a_new_file.txt") == -1);

    int fd_buff[10000];
    printf("Opening files until failure. This might take a bit...\n");
    int i;
    for (i = 0; i < 10000; i++) {
        fd_buff[i] = sos_sys_open("a_new_file.txt", O_RDONLY);
        if (fd_buff[i] == -1) {
            break;
        }
    }

    assert(i < 10000);

    printf("Closing all open files. This might take a bit...\n");
    for (i = 0; i < 10000; i++) {
        if (fd_buff[i] == -1) {
            break;
        }

        assert(sos_sys_close(fd_buff[i]) == 0);
    }

    fd = sos_sys_open("a_new_file.txt", O_RDONLY);
    assert(fd != -1);
    assert(sos_sys_close(fd) == 0);
}

void memory_errors() {
    int dummy_var;
    void *heap_end = sbrk(0);

    /* This is implementation defined. Your stack may be before your heap. */
    assert(heap_end < (void *)&dummy_var);
    assert(sbrk((void *)&dummy_var - heap_end) == -1);
    assert(sbrk(0) == heap_end);

    assert(sbrk(~0) == -1);
    /* We haven't called malloc yet so this should fail. */
    assert(sbrk(-1) == -1);
    assert(sbrk(0) == heap_end);

    assert(sbrk(INT_MAX) == -1);
    assert(sbrk(INT_MIN) == -1);
    assert(sbrk(1 << 31) == -1);
    assert(sbrk(0) == heap_end);

    uint32_t i = 0;
    int inc = (1 << 30);
    while (inc != 0) {
        while (sbrk(inc) != -1) {
            i++;
        }
        inc >>= 1;
    }

    assert(sbrk(INT_MIN) == -1);

    inc = (1 << 30);
    while (inc != 0) {
        while (sbrk(-inc) != -1) {
            i--;
        }
        inc >>= 1;
    }

    assert(i == 0);
    assert(sbrk(0) == heap_end);

    /* We increment by a non page sized value. */
    assert(sbrk(32) == -1);

    // TODO(karl): write mmap tests.
}

void process_errors() {
    char path[2 * MAX_PATH_LENGTH];
    for (int i = 0; i < 2 * MAX_PATH_LENGTH; i++) {
        path[i] = 'a';
    }

    assert(sos_process_create(NULL) == -1);
    assert(sos_process_create((char *)1000) == -1);
    assert(sos_process_create((char *)~0) == -1);
    assert(sos_process_create("") == -1);
    assert(sos_process_create("non_existant_process") == -1);
    assert(sos_process_create(path) == -1);
    path[MAX_PATH_LENGTH] = '\0';
    assert(sos_process_create(path) == -1);

    assert(sos_process_delete(-1) == -1);
    assert(sos_process_delete(INT_MIN) == -1);
    assert(sos_process_delete(INT_MAX) == -1);
    assert(sos_process_delete(sos_my_id() + 1) == -1);

    sos_process_t process_buff[100];
    /* Not an error but a corner case. */
    assert(sos_process_status(process_buff, 0) == 0);
    assert(sos_process_status(process_buff, 100) == 1);
    assert(sos_process_status(NULL, 100) == 0);
    assert(sos_process_status((void *)1000, 100) == 0);
    assert(sos_process_status((void *)~0, 100) == 0);
    assert(sos_process_status(sbrk(0), 1) == 0);
    assert(sos_process_status((void *)"read only string", 1) == 0);

    assert(sos_process_wait(sos_my_id() + 1) == -1);
    assert(sos_process_wait(INT_MIN) == -1);
    assert(sos_process_wait(INT_MAX) == -1);
    /* Implementation defined this might be allowed behaviour. */
    assert(sos_process_wait(sos_my_id()) == -1);

    pid_t pid = sos_process_create("error_test");
    assert(pid != -1);
    assert(sos_process_wait(pid) == pid);
    assert(sos_process_wait(pid) == -1);

    pid = sos_process_create("error_test");
    assert(pid != -1);
    assert(sos_process_delete(pid) == 0);
    assert(sos_process_delete(pid) == -1);

    pid = sos_process_create("error_test");
    assert(pid != -1);
    assert(sos_process_delete(pid) == 0);
    assert(sos_process_wait(pid) == -1);

    pid = sos_process_create("error_test");
    assert(pid != -1);
    assert(sos_process_wait(pid) == pid);
    assert(sos_process_delete(pid) == -1);

    pid = sos_process_create("error_test");
    assert(pid != -1);
    sos_sys_usleep(10000);
    assert(sos_process_create("error_test") != pid);
    assert(sos_process_wait(pid) == pid);
}

void crash_errors() {
    //printf("%c\n", *((char *)NULL));
    //*((char *)NULL) = 2;
    //*"Read only string" = 13;
    printf("%c\n", *((char *)sbrk(0)));
    //*((char *)sbrk(0)) = 2;
}

int main() {
    /* Implementation defined. Set this to your initial id. */
    if (sos_my_id() != 2) {
        printf("I am a child with pid: %d.\n", sos_my_id());
        /* Try to delete our parent. Once again this is implementation defined.*/
        assert(sos_process_delete(sos_my_id() - 1) == -1);
        assert(sos_process_wait(sos_my_id() - 1) == -1);
        printf("Child test exited successfully.\n");
        return 0;
    }

    printf("Running timer error tests.\n");
    timer_errors();
    printf("Timer error tests passed.\n");

    printf("Running file error tests.\n");
    file_errors();
    printf("File error tests passed.\n");

    printf("Running memory error tests.\n");
    printf("Warning: Here be implementation specific dragons.\n");
    memory_errors();
    printf("Memory error tests passed.\n");

    printf("Running process error tests.\n");
    process_errors();
    printf("Process error tests passed.\n");

    // TODO(karl): Write share vm tests.

    printf("Running crash tests. You need to manually comment out individual lines.\n");
    crash_errors();
    assert(!"Crash tests failed you should never get here!\n");

    return 0;
}

