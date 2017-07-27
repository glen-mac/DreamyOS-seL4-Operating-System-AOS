/* @LICENSE(MUSLC_MIT) */

#include <sys/epoll.h>
#include "internal/syscall.h"

int epoll_ctl(int fd, int op, int fd2, struct epoll_event *ev)
{
	return syscall(SYS_epoll_ctl, fd, op, fd2, ev);
}
