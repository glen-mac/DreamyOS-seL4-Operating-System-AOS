/* @LICENSE(MUSLC_MIT) */

#include <termios.h>
#include <sys/ioctl.h>
#include "libc.h"
#include "internal/syscall.h"

int tcdrain(int fd)
{
	return syscall_cp(SYS_ioctl, fd, TCSBRK, 1);
}
