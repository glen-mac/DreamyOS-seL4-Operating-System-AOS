/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"

ssize_t readlink(const char *path, char *buf, size_t bufsize)
{
	return syscall(SYS_readlink, path, buf, bufsize);
}
