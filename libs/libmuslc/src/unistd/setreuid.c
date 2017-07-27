/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"
#include "libc.h"

int setreuid(uid_t ruid, uid_t euid)
{
	return __setxid(SYS_setreuid, ruid, euid, 0);
}
