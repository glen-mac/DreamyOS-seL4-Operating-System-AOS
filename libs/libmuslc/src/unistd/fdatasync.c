/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"

int fdatasync(int fd)
{
	return syscall(SYS_fdatasync, fd);
}
