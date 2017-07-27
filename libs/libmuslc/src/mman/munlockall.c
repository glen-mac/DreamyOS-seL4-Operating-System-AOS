/* @LICENSE(MUSLC_MIT) */

#include <sys/mman.h>
#include "internal/syscall.h"

int munlockall(void)
{
	return syscall(SYS_munlockall);
}
