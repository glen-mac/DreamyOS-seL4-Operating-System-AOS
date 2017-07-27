/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"

pid_t getppid(void)
{
	return __syscall(SYS_getppid);
}
