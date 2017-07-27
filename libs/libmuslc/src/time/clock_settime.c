/* @LICENSE(MUSLC_MIT) */

#include <time.h>
#include "internal/syscall.h"

int clock_settime(clockid_t clk, const struct timespec *ts)
{
	return syscall(SYS_clock_settime, clk, ts);
}
