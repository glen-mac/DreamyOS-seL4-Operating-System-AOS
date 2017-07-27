/* @LICENSE(MUSLC_MIT) */

#include <sys/times.h>
#include "internal/syscall.h"

clock_t times(struct tms *tms)
{
	return syscall(SYS_times, tms);
}
