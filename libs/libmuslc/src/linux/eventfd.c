/* @LICENSE(MUSLC_MIT) */

#include <sys/eventfd.h>
#include "internal/syscall.h"

int eventfd(unsigned int count, int flags)
{
	return syscall(flags ? SYS_eventfd2 : SYS_eventfd, count, flags);
}
