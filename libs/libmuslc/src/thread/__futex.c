/* @LICENSE(MUSLC_MIT) */

#include "futex.h"
#include "internal/syscall.h"

int __futex(volatile int *addr, int op, int val, void *ts)
{
	return syscall(SYS_futex, addr, op, val, ts);
}
