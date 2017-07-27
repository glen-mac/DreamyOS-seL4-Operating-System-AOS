/* @LICENSE(MUSLC_MIT) */

#include <sched.h>
#include "internal/syscall.h"

int sched_getparam(pid_t pid, struct sched_param *param)
{
	return syscall(SYS_sched_getparam, pid, param);
}
