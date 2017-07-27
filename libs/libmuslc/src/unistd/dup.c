/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"

int dup(int fd)
{
	return syscall(SYS_dup, fd);
}
