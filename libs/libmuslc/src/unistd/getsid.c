/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"

pid_t getsid(pid_t pid)
{
	return syscall(SYS_getsid, pid);
}
