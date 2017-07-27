/* @LICENSE(MUSLC_MIT) */

#include <stdint.h>
#include "internal/syscall.h"

void *sbrk(intptr_t inc)
{
	unsigned long cur = syscall(SYS_brk, 0);
	if (inc && syscall(SYS_brk, cur+inc) != cur+inc) return (void *)-1;
	return (void *)cur;
}
