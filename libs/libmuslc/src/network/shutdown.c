/* @LICENSE(MUSLC_MIT) */

#include <sys/socket.h>
#include "internal/syscall.h"

int shutdown(int fd, int how)
{
	return socketcall(shutdown, fd, how, 0, 0, 0, 0);
}
