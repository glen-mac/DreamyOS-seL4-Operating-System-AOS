/* @LICENSE(MUSLC_MIT) */

#include <sys/epoll.h>
#include "internal/syscall.h"

int epoll_create1(int flags)
{
	return syscall(SYS_epoll_create1, flags);
}
