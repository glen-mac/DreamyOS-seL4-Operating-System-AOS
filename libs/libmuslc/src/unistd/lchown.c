/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"

int lchown(const char *path, uid_t uid, gid_t gid)
{
	return syscall(SYS_lchown, path, uid, gid);
}
