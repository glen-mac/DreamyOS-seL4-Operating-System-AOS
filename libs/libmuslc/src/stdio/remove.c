/* @LICENSE(MUSLC_MIT) */

#include <stdio.h>
#include <errno.h>
#include "internal/syscall.h"

int remove(const char *path)
{
	int r = syscall(SYS_unlink, path);
	return (r && errno == EISDIR) ? syscall(SYS_rmdir, path) : r;
}
