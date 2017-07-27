/* @LICENSE(MUSLC_MIT) */

#define _GNU_SOURCE
#include <unistd.h>
#include "internal/syscall.h"
#include "libc.h"

int setresgid(gid_t rgid, gid_t egid, gid_t sgid)
{
	return __setxid(SYS_setresgid, rgid, egid, sgid);
}
