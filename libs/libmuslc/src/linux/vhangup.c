/* @LICENSE(MUSLC_MIT) */

#define _GNU_SOURCE
#include <unistd.h>
#include "internal/syscall.h"

int vhangup(void)
{
	return syscall(SYS_vhangup);
}
