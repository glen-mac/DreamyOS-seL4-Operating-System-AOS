/* @LICENSE(MUSLC_MIT) */

#include <sys/mount.h>
#include "internal/syscall.h"

int umount(const char *special)
{
	return syscall(SYS_umount2, special, 0);
}
