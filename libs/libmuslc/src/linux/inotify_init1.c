/* @LICENSE(MUSLC_MIT) */

#include <sys/inotify.h>
#include "internal/syscall.h"

int inotify_init1(int flags)
{
	return syscall(SYS_inotify_init1, flags);
}
