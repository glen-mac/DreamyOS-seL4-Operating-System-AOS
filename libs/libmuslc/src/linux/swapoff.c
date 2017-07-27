/* @LICENSE(MUSLC_MIT) */

#include <sys/swap.h>
#include "internal/syscall.h"

int swapoff(const char *path)
{
	return syscall(SYS_swapoff, path);
}
