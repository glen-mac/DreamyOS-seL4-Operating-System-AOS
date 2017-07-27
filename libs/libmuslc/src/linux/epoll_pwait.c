/* @LICENSE(MUSLC_MIT) */

#include <sys/epoll.h>
#include "internal/syscall.h"

int epoll_pwait(int fd, struct epoll_event *ev, int cnt, int to, const sigset_t *sigs)
{
	return syscall(SYS_epoll_pwait, fd, ev, cnt, to, sigs, __SYSCALL_SSLEN);
}
