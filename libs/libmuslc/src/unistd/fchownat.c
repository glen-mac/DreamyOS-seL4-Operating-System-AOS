/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"

int fchownat(int fd, const char *path, uid_t uid, gid_t gid, int flag)
{
	return syscall(SYS_fchownat, fd, path, uid, gid, flag);
}
