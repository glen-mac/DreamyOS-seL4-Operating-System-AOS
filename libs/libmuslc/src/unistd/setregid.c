/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"
#include "libc.h"

int setregid(gid_t rgid, gid_t egid)
{
	return __setxid(SYS_setregid, rgid, egid, 0);
}
