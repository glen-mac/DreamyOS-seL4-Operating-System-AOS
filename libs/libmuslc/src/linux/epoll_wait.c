/* @LICENSE(MUSLC_MIT) */

#include <sys/epoll.h>
#include "internal/syscall.h"

int epoll_wait(int fd, struct epoll_event *ev, int cnt, int to)
{
	return syscall(SYS_epoll_wait, fd, ev, cnt, to);
}
