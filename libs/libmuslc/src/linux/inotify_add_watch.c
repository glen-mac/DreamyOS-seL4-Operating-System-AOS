/* @LICENSE(MUSLC_MIT) */

#include <sys/inotify.h>
#include "internal/syscall.h"

int inotify_add_watch(int fd, const char *pathname, uint32_t mask)
{
	return syscall(SYS_inotify_add_watch, fd, pathname, mask);
}
