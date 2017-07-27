/* @LICENSE(MUSLC_MIT) */

#include <sched.h>
#include "internal/syscall.h"

int sched_getscheduler(pid_t pid)
{
	return syscall(SYS_sched_getscheduler, pid);
}
