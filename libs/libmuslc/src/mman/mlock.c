/* @LICENSE(MUSLC_MIT) */

#include <sys/mman.h>
#include "internal/syscall.h"

int mlock(const void *addr, size_t len)
{
	return syscall(SYS_mlock, addr, len);
}
