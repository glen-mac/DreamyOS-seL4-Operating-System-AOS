/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"

int setgroups(size_t count, const gid_t list[])
{
	return syscall(SYS_setgroups, count, list);
}
