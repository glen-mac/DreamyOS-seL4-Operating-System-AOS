/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"

gid_t getegid(void)
{
	return __syscall(SYS_getegid);
}
