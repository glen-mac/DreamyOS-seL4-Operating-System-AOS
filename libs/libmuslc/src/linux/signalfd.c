/* @LICENSE(MUSLC_MIT) */

#include <sys/signalfd.h>
#include "internal/syscall.h"

int signalfd(int fd, const sigset_t *sigs, int flags)
{
	return syscall(SYS_signalfd, fd, sigs, __SYSCALL_SSLEN);
}
