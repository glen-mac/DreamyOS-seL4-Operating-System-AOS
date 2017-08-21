/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sos.h>

#include <sel4/sel4.h>

int
sos_sys_open(const char *path, fmode_t mode)
{
    assert(!"You need to implement this");
    return -1;
}

int
sos_sys_read(int file, char *buf, size_t nbyte)
{
    assert(!"You need to implement this");
    return -1;
}

int
sos_sys_write(int file, const char *buf, size_t nbyte)
{
    assert(!"You need to implement this");
    return -1;
}

int
sos_sys_close(int file)
{
    assert(!"You need to implement this");
    return -1; 
}

int
sos_getdirent(int pos, char *name, size_t nbyte)
{
    assert(!"You need to implement this");
    return -1;
}

int
sos_stat(const char *path, sos_stat_t *buf)
{
    assert(!"You need to implement this");
    return -1;
}

pid_t
sos_process_create(const char *path)
{
    assert(!"You need to implement this");
    return -1;
}

int
sos_process_delete(pid_t pid)
{
    assert(!"You need to implement this");
    return -1;
}

pid_t
sos_my_id(void)
{
    assert(!"You need to implement this");
    return -1;
}

int
sos_process_status(sos_process_t *processes, unsigned max)
{
    assert(!"You need to implement this");
    return -1;
}

pid_t
sos_process_wait(pid_t pid)
{
    assert(!"You need to implement this");
    return -1;
}

void
sos_sys_usleep(int msec)
{
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 2);
    seL4_SetTag(tag);
    seL4_SetMR(0, SOS_SYS_USLEEP);  /* Syscall number */
    seL4_SetMR(1, msec);            /* # of msec to sleep */
    seL4_Call(SOS_IPC_EP_CAP, tag);
       
    /* at this point SOS has slept the time period */
    return;
}

int64_t
sos_sys_time_stamp(void)
{
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetTag(tag);
    seL4_SetMR(0, SOS_SYS_TIME_STAMP); /* Syscall number */
    seL4_Call(SOS_IPC_EP_CAP, tag);

    /* Receive back the result */
    uint32_t ts_upper = seL4_GetMR(0);
    uint32_t ts_lower = seL4_GetMR(1);
    uint64_t ts = (ts_upper << 32) | ts_lower;
    return ts; 
}

/*
 * sys_brk call
 * @param newbrk: the desired new brk address - if this value is 0, the user
 * is calling in a fashion similiar to sbrk and wants the current heap brk
 * value returned, otherwise they want it set (if possible)
 * @returns: the current brk value (after query or change)
 */
seL4_Word sos_sys_brk(seL4_Word newbrk) {
    int ret_val;

    /* need two registers, one for syscall the other for newbrk */
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 2);
    seL4_SetTag(tag);
    seL4_SetMR(0, SOS_SYS_BRK); /* Syscall number */
    seL4_SetMR(1, newbrk); /* newbrk */
    seL4_Call(SOS_IPC_EP_CAP, tag);

    ret_val = seL4_GetMR(1); /* Receive back the result */
    return ret_val; /* could contain newbrk, or the original brk */
}
