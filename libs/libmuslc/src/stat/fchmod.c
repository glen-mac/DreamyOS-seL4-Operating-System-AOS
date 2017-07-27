/* @LICENSE(MUSLC_MIT) */

#include <sys/stat.h>
#include "internal/syscall.h"

int fchmod(int fd, mode_t mode)
{
	return syscall(SYS_fchmod, fd, mode);
}
