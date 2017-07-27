/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"

int sethostname(const char *name, size_t len)
{
	return syscall(SYS_sethostname, name, len);
}
