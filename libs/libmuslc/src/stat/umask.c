/* @LICENSE(MUSLC_MIT) */

#include <sys/stat.h>
#include "internal/syscall.h"

mode_t umask(mode_t mode)
{
	return syscall(SYS_umask, mode);
}
