/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"

pid_t setsid(void)
{
	return syscall(SYS_setsid);
}
