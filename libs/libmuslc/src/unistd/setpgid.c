/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"

int setpgid(pid_t pid, pid_t pgid)
{
	return syscall(SYS_setpgid, pid, pgid);
}
