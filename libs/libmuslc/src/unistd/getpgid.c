/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"

pid_t getpgid(pid_t pid)
{
	return syscall(SYS_getpgid, pid);
}
