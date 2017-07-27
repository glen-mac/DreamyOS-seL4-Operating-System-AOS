/* @LICENSE(MUSLC_MIT) */

#include <sys/socket.h>
#include "internal/syscall.h"

int getsockname(int fd, struct sockaddr *addr, socklen_t *len)
{
	return socketcall(getsockname, fd, addr, len, 0, 0, 0);
}
