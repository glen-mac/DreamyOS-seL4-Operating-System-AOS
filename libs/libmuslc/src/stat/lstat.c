/* @LICENSE(MUSLC_MIT) */

#include <sys/stat.h>
#include "internal/syscall.h"
#include "libc.h"

int lstat(const char *path, struct stat *buf)
{
	return syscall(SYS_lstat, path, buf);
}

LFS64(lstat);
