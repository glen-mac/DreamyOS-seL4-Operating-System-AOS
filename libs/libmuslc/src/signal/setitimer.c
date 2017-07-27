/* @LICENSE(MUSLC_MIT) */

#include <sys/time.h>
#include "internal/syscall.h"

int setitimer(int which, const struct itimerval *new, struct itimerval *old)
{
	return syscall(SYS_setitimer, which, new, old);
}
