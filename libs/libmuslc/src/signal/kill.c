/* @LICENSE(MUSLC_MIT) */

#include <signal.h>
#include "internal/syscall.h"

int kill(pid_t pid, int sig)
{
	return syscall(SYS_kill, pid, sig);
}
