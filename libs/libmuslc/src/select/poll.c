/* @LICENSE(MUSLC_MIT) */

#include <poll.h>
#include "internal/syscall.h"
#include "libc.h"

int poll(struct pollfd *fds, nfds_t n, int timeout)
{
	return syscall_cp(SYS_poll, fds, n, timeout);
}
