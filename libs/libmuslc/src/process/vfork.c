/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"
#include "libc.h"

pid_t __vfork(void)
{
	/* vfork syscall cannot be made from C code */
	return syscall(SYS_fork);
}

weak_alias(__vfork, vfork);
