/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"

int chroot(const char *path)
{
	return syscall(SYS_chroot, path);
}
