/* @LICENSE(MUSLC_MIT) */

#include "internal/syscall.h"

int capset(void *a, void *b)
{
	return syscall(SYS_capset, a, b);
}

int capget(void *a, void *b)
{
	return syscall(SYS_capget, a, b);
}
