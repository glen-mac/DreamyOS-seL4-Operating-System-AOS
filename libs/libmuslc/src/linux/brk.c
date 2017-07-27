/* @LICENSE(MUSLC_MIT) */

#include "internal/syscall.h"

int brk(void *end)
{
	return -(syscall(SYS_brk, end) != (unsigned long)end);
}
