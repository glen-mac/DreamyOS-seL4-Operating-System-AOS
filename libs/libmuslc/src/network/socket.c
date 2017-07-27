/* @LICENSE(MUSLC_MIT) */

#include <sys/socket.h>
#include "internal/syscall.h"

int socket(int domain, int type, int protocol)
{
	return socketcall(socket, domain, type, protocol, 0, 0, 0);
}
