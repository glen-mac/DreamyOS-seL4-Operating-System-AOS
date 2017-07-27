/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"
#include "libc.h"

int seteuid(uid_t euid)
{
	return __setxid(SYS_setresuid, -1, euid, -1);
}
