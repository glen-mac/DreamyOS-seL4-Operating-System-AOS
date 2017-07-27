/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"
#include "libc.h"

int setgid(gid_t gid)
{
	return __setxid(SYS_setgid, gid, 0, 0);
}
