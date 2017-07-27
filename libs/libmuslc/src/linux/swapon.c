/* @LICENSE(MUSLC_MIT) */

#include <sys/swap.h>
#include "internal/syscall.h"

int swapon(const char *path, int flags)
{
	return syscall(SYS_swapon, path, flags);
}
