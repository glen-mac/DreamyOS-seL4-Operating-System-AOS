/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"
#include "libc.h"

int pause(void)
{
	return syscall_cp(SYS_pause);
}
