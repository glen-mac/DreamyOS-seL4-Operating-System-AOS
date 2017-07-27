/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include <time.h>
#include "internal/syscall.h"
#include "libc.h"

int nanosleep(const struct timespec *req, struct timespec *rem)
{
	return syscall_cp(SYS_nanosleep, req, rem);
}
