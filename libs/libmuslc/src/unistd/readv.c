/* @LICENSE(MUSLC_MIT) */

#include <sys/uio.h>
#include "internal/syscall.h"
#include "libc.h"

ssize_t readv(int fd, const struct iovec *iov, int count)
{
	return syscall_cp(SYS_readv, fd, iov, count);
}
