/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "libc.h"
#include "internal/syscall.h"

int setegid(gid_t egid)
{
	return __setxid(SYS_setresgid, -1, egid, -1);
}
