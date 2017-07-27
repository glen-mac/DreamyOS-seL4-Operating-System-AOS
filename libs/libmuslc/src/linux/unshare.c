/* @LICENSE(MUSLC_MIT) */

#define _GNU_SOURCE
#include <sched.h>
#include "internal/syscall.h"

int unshare(int flags)
{
	return syscall(SYS_unshare, flags);
}
