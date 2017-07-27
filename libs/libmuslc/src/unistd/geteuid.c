/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"

uid_t geteuid(void)
{
	return __syscall(SYS_geteuid);
}
