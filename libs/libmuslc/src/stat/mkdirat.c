/* @LICENSE(MUSLC_MIT) */

#include <sys/stat.h>
#include "internal/syscall.h"

int mkdirat(int fd, const char *path, mode_t mode)
{
	return syscall(SYS_mkdirat, fd, path, mode);
}
