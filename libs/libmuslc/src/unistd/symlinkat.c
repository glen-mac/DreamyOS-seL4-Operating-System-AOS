/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"

int symlinkat(const char *existing, int fd, const char *new)
{
	return syscall(SYS_symlinkat, existing, fd, new);
}
