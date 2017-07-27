/* @LICENSE(MUSLC_MIT) */

#include <semaphore.h>
#include <sys/mman.h>

int sem_unlink(const char *name)
{
	return shm_unlink(name);
}
