/* @LICENSE(MUSLC_MIT) */

#include "pthread_impl.h"
#include <stdio.h>

static long sccp(long nr, long u, long v, long w, long x, long y, long z)
{

    return (__syscall)(nr, u, v, w, x, y, z);
}

weak_alias(sccp, __syscall_cp);

static void dummy()
{
}

weak_alias(dummy, __testcancel);
