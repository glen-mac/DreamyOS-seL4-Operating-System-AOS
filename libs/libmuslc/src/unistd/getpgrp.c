/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"

pid_t getpgrp(void)
{
	return __syscall(SYS_getpgrp);
}
