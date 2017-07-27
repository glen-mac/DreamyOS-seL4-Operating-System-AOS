/* @LICENSE(MUSLC_MIT) */

#include <sys/socket.h>
#include "internal/syscall.h"

int socketpair(int domain, int type, int protocol, int fd[2])
{
	return socketcall(socketpair, domain, type, protocol, fd, 0, 0);
}
