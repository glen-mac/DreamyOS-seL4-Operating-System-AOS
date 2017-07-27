/* @LICENSE(MUSLC_MIT) */


#include <stdlib.h>
#include <signal.h>
#include "internal/syscall.h"

void abort(void)
{
	raise(SIGABRT);
	raise(SIGKILL);
	for (;;);
}
