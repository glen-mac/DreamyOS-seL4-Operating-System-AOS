/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"
#include "libc.h"

int setfsgid(gid_t gid)
{
	return syscall(SYS_setfsgid, gid);
}
