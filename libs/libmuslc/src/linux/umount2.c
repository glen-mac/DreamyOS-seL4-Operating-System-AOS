/* @LICENSE(MUSLC_MIT) */

#include <sys/mount.h>
#include "internal/syscall.h"

int umount2(const char *special, int flags)
{
	return syscall(SYS_umount2, special, flags);
}
