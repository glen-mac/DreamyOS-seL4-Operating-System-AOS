/* @LICENSE(MUSLC_MIT) */

/* The system call distribut interface. */

#include <assert.h>
#include <stddef.h>
#include <bits/syscall.h>
#include "syscall_sel4.h"

muslc_syscall_array_t *__muslc_syscall_ptr_table;

long __syscall(long n, ...)
{
    long ret = 0;
    va_list ap;
    va_start(ap, n);

    assert (n >=0 && n < SYSCALL_MUSLC_NUM);
    assert(__muslc_syscall_ptr_table);
    assert((*__muslc_syscall_ptr_table)[n]);

    ret = (*__muslc_syscall_ptr_table)[n](ap);

    va_end(ap);

    return ret;
}
