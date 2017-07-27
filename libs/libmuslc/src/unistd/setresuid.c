/* @LICENSE(MUSLC_MIT) */

#define _GNU_SOURCE
#include <unistd.h>
#include "internal/syscall.h"
#include "libc.h"

int setresuid(uid_t ruid, uid_t euid, uid_t suid)
{
	return __setxid(SYS_setresuid, ruid, euid, suid);
}
