/* @LICENSE(MUSLC_MIT) */

#include "internal/syscall.h"

struct sysinfo;

int sysinfo(struct sysinfo *info)
{
	return syscall(SYS_sysinfo, info);
}
