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

/*
 * Macros to count the number of variadic arguments 
 * https://stackoverflow.com/questions/11317474/macro-to-count-number-of-arguments
 */
#define PP_NARG(...) PP_NARG_(__VA_ARGS__,PP_RSEQ_N())
#define PP_NARG_(...) PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N( \
     _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
    _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
    _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
    _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
    _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
    _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
    _61,_62,_63,  N, ...) N
#define PP_RSEQ_N() \
    63,62,61,60,                   \
    59,58,57,56,55,54,53,52,51,50, \
    49,48,47,46,45,44,43,42,41,40, \
    39,38,37,36,35,34,33,32,31,30, \
    29,28,27,26,25,24,23,22,21,20, \
    19,18,17,16,15,14,13,12,11,10, \
     9, 8, 7, 6, 5, 4, 3, 2, 1, 0

#define MAKE_SYSCALL(...) make_syscall(PP_NARG(__VA_ARGS__) - 1, __VA_ARGS__)

/*
 * Make a syscall to SOS
 * @param nargs, number of arguments in the variadic arguments
 * @param syscall_no, the number of the syscall
 * @param varargs, the varadic arguments for the syscall
 */
static void
make_syscall(const seL4_Word nargs, const seL4_Word syscall_no, ...)
{
    va_list arg_list;
    va_start(arg_list, syscall_no);

    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 1 + nargs);
    seL4_SetTag(tag);
    seL4_SetMR(0, syscall_no);

    for (size_t i = 0; i < nargs; ++i)
        seL4_SetMR(1 + i, (seL4_Word)va_arg(arg_list, seL4_Word));

    va_end(arg_list);
    seL4_Call(SOS_IPC_EP_CAP, tag);
}

int
sos_sys_open(const char *path, fmode_t mode)
{
    MAKE_SYSCALL(SOS_SYS_OPEN, path, mode);
    return (int)seL4_GetMR(0); /* Receive back the fd */
}

int
sos_sys_read(int file, char *buf, size_t nbyte)
{
    MAKE_SYSCALL(SOS_SYS_READ, file, buf, nbyte);
    return (int)seL4_GetMR(0); /* Receive nbytes read */
}

int
sos_sys_write(int file, const char *buf, size_t nbyte)
{
    MAKE_SYSCALL(SOS_SYS_WRITE, file, buf, nbyte);
    return (int)seL4_GetMR(0); /* Receive nbytes written */
}

int
sos_sys_close(int file)
{
    MAKE_SYSCALL(SOS_SYS_CLOSE, file);
    return (int)seL4_GetMR(0); /* -1 on error, 0 on success */
}

int
sos_getdirent(int pos, char *name, size_t nbyte)
{
    MAKE_SYSCALL(SOS_SYS_GETDIRENT, pos, name, nbyte);
    return (int)seL4_GetMR(0);
}

int
sos_stat(const char *name, sos_stat_t *buf)
{
    MAKE_SYSCALL(SOS_SYS_STAT, name, buf);
    return (int)seL4_GetMR(0); /* -1 on error, 0 on success */
}

pid_t
sos_process_create(const char *path)
{
    MAKE_SYSCALL(SOS_SYS_PROC_CREATE, path, strlen(path));
    return (pid_t)seL4_GetMR(0); /* -1 on error, 0 on success */
}

int
sos_process_delete(pid_t pid)
{
    MAKE_SYSCALL(SOS_SYS_PROC_DELETE, pid);
    return (int)seL4_GetMR(0); /* -1 on error, 0 on success */
}

pid_t
sos_my_id(void)
{
    MAKE_SYSCALL(SOS_SYS_PROC_ID);
    return (pid_t)seL4_GetMR(0);
}

int
sos_process_status(sos_process_t *processes, unsigned max)
{
    MAKE_SYSCALL(SOS_SYS_PROC_STATUS, processes, max);
    return (int)seL4_GetMR(0); /* num processes returned */
}

pid_t
sos_process_wait(pid_t pid)
{
    MAKE_SYSCALL(SOS_SYS_PROC_WAIT, pid);
    return (int)seL4_GetMR(0); /* PID of process which exited */
}

void
sos_sys_usleep(int msec)
{
    MAKE_SYSCALL(SOS_SYS_USLEEP, msec);
    return; /* At this point SOS has slept the time period */
}

int64_t
sos_sys_time_stamp(void)
{
    MAKE_SYSCALL(SOS_SYS_TIME_STAMP);

    /* Receive back the result */
    uint32_t ts_upper = seL4_GetMR(0);
    uint32_t ts_lower = seL4_GetMR(1);
    return (uint64_t)ts_lower | ((uint64_t)ts_upper << 32);
}

seL4_Word
sos_sys_brk(seL4_Word newbrk)
{
    MAKE_SYSCALL(SOS_SYS_BRK, newbrk);
    return seL4_GetMR(1); /* could contain newbrk, or the original brk */
}

void
sos_sys_exit(void)
{
    MAKE_SYSCALL(SOS_SYS_EXIT);
}
