/* @LICENSE(MUSLC_MIT) */

#include <stdio.h>
#include "internal/syscall.h"

int rename(const char *old, const char *new)
{
	return syscall(SYS_rename, old, new);
}
