/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"

pid_t getpid(void)
{
	return __syscall(SYS_getpid);
}
