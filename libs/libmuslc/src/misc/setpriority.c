/* @LICENSE(MUSLC_MIT) */

#include <sys/resource.h>
#include "internal/syscall.h"

int setpriority(int which, id_t who, int prio)
{
	return syscall(SYS_getpriority, which, who, prio);
}
