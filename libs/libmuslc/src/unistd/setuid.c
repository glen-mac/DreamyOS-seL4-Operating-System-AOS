/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"
#include "libc.h"

int setuid(uid_t uid)
{
	return __setxid(SYS_setuid, uid, 0, 0);
}
