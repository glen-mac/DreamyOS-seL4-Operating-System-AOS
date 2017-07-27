/* @LICENSE(MUSLC_MIT) */

#include <sys/sem.h>
#include "internal/syscall.h"
#include "ipc.h"

int semget(key_t key, int n, int fl)
{
#ifdef SYS_semget
	return syscall(SYS_semget, key, n, fl);
#else
	return syscall(SYS_ipc, IPCOP_semget, key, n, fl);
#endif
}
