/* @LICENSE(MUSLC_MIT) */

#include <sys/mman.h>
#include "internal/syscall.h"

int mlockall(int flags)
{
	return syscall(SYS_mlockall, flags);
}
