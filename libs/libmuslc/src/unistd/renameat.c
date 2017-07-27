/* @LICENSE(MUSLC_MIT) */

#include <stdio.h>
#include "internal/syscall.h"

int renameat(int oldfd, const char *old, int newfd, const char *new)
{
	return syscall(SYS_renameat, oldfd, old, newfd, new);
}
