/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"

int unlink(const char *path)
{
	return syscall(SYS_unlink, path);
}
