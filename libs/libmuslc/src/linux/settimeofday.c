/* @LICENSE(MUSLC_MIT) */

#include <sys/time.h>
#include "internal/syscall.h"

int settimeofday(const struct timeval *tv, void *tz)
{
	return syscall(SYS_settimeofday, tv, 0);
}
