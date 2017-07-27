/* @LICENSE(MUSLC_MIT) */

#include <sys/sem.h>
#include "internal/syscall.h"
#include "ipc.h"

int semop(int id, struct sembuf *buf, size_t n)
{
#ifdef SYS_semop
	return syscall(SYS_semop, id, buf, n);
#else
	return syscall(SYS_ipc, IPCOP_semop, id, n, 0, buf);
#endif
}
