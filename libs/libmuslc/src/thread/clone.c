/* @LICENSE(MUSLC_MIT) */

#include "pthread_impl.h"

int clone(int (*func)(void *), void *stack, int flags, void *arg, ...)
{

    return __syscall(SYS_clone, func, stack, flags, arg);
}


