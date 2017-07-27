/* @LICENSE(MUSLC_MIT) */

#include <sys/time.h>
#include "internal/syscall.h"

int getitimer(int which, struct itimerval *old)
{
	return syscall(SYS_getitimer, which, old);
}
