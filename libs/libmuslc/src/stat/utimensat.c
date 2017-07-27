/* @LICENSE(MUSLC_MIT) */

#include <sys/stat.h>
#include "internal/syscall.h"

int utimensat(int fd, const char *path, const struct timespec times[2], int flags)
{
	return syscall(SYS_utimensat, fd, path, times, flags);
}
