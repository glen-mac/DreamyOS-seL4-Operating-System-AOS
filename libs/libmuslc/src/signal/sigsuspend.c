/* @LICENSE(MUSLC_MIT) */

#include <signal.h>
#include "internal/syscall.h"
#include "libc.h"

int sigsuspend(const sigset_t *mask)
{
	return syscall_cp(SYS_rt_sigsuspend, mask, __SYSCALL_SSLEN);
}
