/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"
#include "libc.h"

ssize_t read(int fd, void *buf, size_t count)
{
	return syscall_cp(SYS_read, fd, buf, count);
}
