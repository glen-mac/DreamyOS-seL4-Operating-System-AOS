/* @LICENSE(MUSLC_MIT) */

#include <sys/utsname.h>
#include <string.h>
#include "internal/syscall.h"

int uname(struct utsname *uts)
{
	return syscall(SYS_uname, uts);
}
