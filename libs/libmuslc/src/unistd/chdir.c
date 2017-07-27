/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"

int chdir(const char *path)
{
	return syscall(SYS_chdir, path);
}
