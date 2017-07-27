/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"

int chown(const char *path, uid_t uid, gid_t gid)
{
	return syscall(SYS_chown, path, uid, gid);
}
