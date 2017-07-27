/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"

int fsync(int fd)
{
	return syscall(SYS_fsync, fd);
}
