/* @LICENSE(MUSLC_MIT) */

#include <sys/socket.h>
#include "internal/syscall.h"

int listen(int fd, int backlog)
{
	return socketcall(listen, fd, backlog, 0, 0, 0, 0);
}
