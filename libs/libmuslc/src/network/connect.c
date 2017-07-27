/* @LICENSE(MUSLC_MIT) */

#include <sys/socket.h>
#include "internal/syscall.h"
#include "libc.h"

int connect(int fd, const struct sockaddr *addr, socklen_t len)
{
	return socketcall_cp(connect, fd, addr, len, 0, 0, 0);
}
