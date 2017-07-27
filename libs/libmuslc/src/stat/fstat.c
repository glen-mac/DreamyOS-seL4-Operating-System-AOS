/* @LICENSE(MUSLC_MIT) */

#include <sys/stat.h>
#include "internal/syscall.h"
#include "libc.h"

int fstat(int fd, struct stat *buf)
{
	return syscall(SYS_fstat, fd, buf);
}

LFS64(fstat);
