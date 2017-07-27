/* @LICENSE(MUSLC_MIT) */

#include <sys/reboot.h>
#include "internal/syscall.h"

int reboot(int type)
{
	return syscall(SYS_reboot, 0xfee1dead, 672274793, type);
}
