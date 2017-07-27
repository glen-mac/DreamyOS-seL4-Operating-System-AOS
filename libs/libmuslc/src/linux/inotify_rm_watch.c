/* @LICENSE(MUSLC_MIT) */

#include <sys/inotify.h>
#include "internal/syscall.h"

int inotify_rm_watch(int fd, uint32_t wd)
{
	return syscall(SYS_inotify_rm_watch, fd, wd);
}
