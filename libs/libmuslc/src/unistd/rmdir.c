/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"

int rmdir(const char *path)
{
	return syscall(SYS_rmdir, path);
}
