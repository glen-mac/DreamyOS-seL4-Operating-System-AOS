/* @LICENSE(MUSLC_MIT) */

#include <signal.h>
#include "internal/syscall.h"

int sigpending(sigset_t *set)
{
	return syscall(SYS_rt_sigpending, set, __SYSCALL_SSLEN);
}
