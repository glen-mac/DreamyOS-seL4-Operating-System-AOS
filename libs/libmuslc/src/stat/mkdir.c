/* @LICENSE(MUSLC_MIT) */

#include <sys/stat.h>
#include "internal/syscall.h"

int mkdir(const char *path, mode_t mode)
{
	return syscall(SYS_mkdir, path, mode);
}
