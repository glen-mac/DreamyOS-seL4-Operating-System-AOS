/* @LICENSE(MUSLC_MIT) */

#include <sys/timex.h>
#include "internal/syscall.h"

int adjtimex(struct timex *tx)
{
	return syscall(SYS_adjtimex, tx);
}
