/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"

uid_t getuid(void)
{
	return __syscall(SYS_getuid);
}
