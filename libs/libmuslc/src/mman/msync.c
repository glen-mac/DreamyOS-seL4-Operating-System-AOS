/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include <sys/mman.h>
#include "internal/syscall.h"

int msync(void *start, size_t len, int flags)
{
	return syscall(SYS_msync, start, len, flags);
}
