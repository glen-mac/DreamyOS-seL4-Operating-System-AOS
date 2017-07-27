/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include "internal/syscall.h"

int pipe(int fd[2])
{
	return syscall(SYS_pipe, fd);
}
