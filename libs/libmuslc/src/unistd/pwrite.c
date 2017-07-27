/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"
#include "libc.h"

ssize_t pwrite(int fd, const void *buf, size_t size, off_t ofs)
{
	return syscall_cp(SYS_pwrite, fd, buf, size, __SYSCALL_LL_O(ofs));
}

LFS64(pwrite);
