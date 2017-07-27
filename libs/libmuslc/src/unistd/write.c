/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"
#include "libc.h"

ssize_t write(int fd, const void *buf, size_t count)
{
	return syscall_cp(SYS_write, fd, buf, count);
}
