/* @LICENSE(MUSLC_MIT) */

#include <sys/io.h>
#include "internal/syscall.h"

#ifdef SYS_iopl
int iopl(int level)
{
	return syscall(SYS_iopl, level);
}
#endif
