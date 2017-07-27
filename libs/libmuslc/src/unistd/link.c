/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"

int link(const char *existing, const char *new)
{
	return syscall(SYS_link, existing, new);
}
