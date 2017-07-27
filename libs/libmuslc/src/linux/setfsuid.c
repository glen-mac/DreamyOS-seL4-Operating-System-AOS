/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"
#include "libc.h"

int setfsuid(uid_t uid)
{
	return syscall(SYS_setfsuid, uid);
}
