/* @LICENSE(MUSLC_MIT) */

#define _GNU_SOURCE
#include <unistd.h>
#include "internal/syscall.h"

int getresuid(uid_t *ruid, uid_t *euid, uid_t *suid)
{
	return syscall(SYS_getresuid, ruid, euid, suid);
}
