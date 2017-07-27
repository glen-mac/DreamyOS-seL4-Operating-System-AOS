/* @LICENSE(MUSLC_MIT) */

#include <sched.h>
#include "internal/syscall.h"
#include "libc.h"

int __yield()
{
	return syscall(SYS_sched_yield);
}

weak_alias(__yield, sched_yield);
