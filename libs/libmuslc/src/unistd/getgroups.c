/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"

int getgroups(int count, gid_t list[])
{
	return syscall(SYS_getgroups, count, list);
}
