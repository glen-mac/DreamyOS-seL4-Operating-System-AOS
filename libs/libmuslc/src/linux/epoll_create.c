/* @LICENSE(MUSLC_MIT) */

#include <sys/epoll.h>
#include "internal/syscall.h"

int epoll_create(int size)
{
	return syscall(SYS_epoll_create, size);
}
