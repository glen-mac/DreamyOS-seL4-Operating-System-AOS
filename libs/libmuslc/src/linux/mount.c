/* @LICENSE(MUSLC_MIT) */

#include <sys/mount.h>
#include "internal/syscall.h"

int mount(const char *special, const char *dir, const char *fstype, unsigned long flags, const void *data)
{
	return syscall(SYS_mount, special, dir, fstype, flags, data);
}
