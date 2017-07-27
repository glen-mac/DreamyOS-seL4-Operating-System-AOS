/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"
#include "libc.h"

int close(int fd)
{
	return syscall_cp(SYS_close, fd);
}
