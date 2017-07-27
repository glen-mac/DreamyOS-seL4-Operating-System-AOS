/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"

void sync(void)
{
	__syscall(SYS_sync);
}
