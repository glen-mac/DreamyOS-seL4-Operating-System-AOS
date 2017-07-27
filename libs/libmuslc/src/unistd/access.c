/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"

int access(const char *filename, int amode)
{
	return syscall(SYS_access, filename, amode);
}
