/* @LICENSE(MUSLC_MIT) */

#include <sys/stat.h>
#include "internal/syscall.h"

int fchmodat(int fd, const char *path, mode_t mode, int flag)
{
	return syscall(SYS_fchmodat, fd, path, mode, flag);
}
