/* @LICENSE(MUSLC_MIT) */

#include <sys/resource.h>
#include <string.h>
#include "internal/syscall.h"

int getrusage(int who, struct rusage *ru)
{
	return syscall(SYS_getrusage, who, ru);
}
