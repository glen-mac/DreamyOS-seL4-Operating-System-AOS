/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"

int faccessat(int fd, const char *filename, int amode, int flag)
{
	return syscall(SYS_faccessat, fd, filename, amode, flag);
}
