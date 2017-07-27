/* @LICENSE(MUSLC_MIT) */

#include <sys/mman.h>
#include "internal/syscall.h"

int munlock(const void *addr, size_t len)
{
	return syscall(SYS_munlock, addr, len);
}
