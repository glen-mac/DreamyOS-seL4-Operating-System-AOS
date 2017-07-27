/* @LICENSE(MUSLC_MIT) */

#include <sys/file.h>
#include "internal/syscall.h"

int flock(int fd, int op)
{
	return syscall(SYS_flock, fd, op);
}
