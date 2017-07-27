/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"

gid_t getgid(void)
{
	return __syscall(SYS_getgid);
}
