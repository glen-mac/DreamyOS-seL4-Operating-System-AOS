/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"

int fchown(int fd, uid_t uid, gid_t gid)
{
	return syscall(SYS_fchown, fd, uid, gid);
}
