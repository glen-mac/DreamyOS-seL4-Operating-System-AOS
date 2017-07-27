/* @LICENSE(MUSLC_MIT) */

#include <sys/stat.h>
#include "internal/syscall.h"

int chmod(const char *path, mode_t mode)
{
	return syscall(SYS_chmod, path, mode);
}
