/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"

int fchdir(int fd)
{
	return syscall(SYS_fchdir, fd);
}
