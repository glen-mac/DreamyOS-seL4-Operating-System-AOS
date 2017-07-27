/* @LICENSE(MUSLC_MIT) */

#include <sys/inotify.h>
#include "internal/syscall.h"

int inotify_init()
{
	return syscall(SYS_inotify_init);
}
