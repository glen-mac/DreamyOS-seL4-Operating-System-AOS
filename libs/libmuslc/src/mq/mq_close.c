/* @LICENSE(MUSLC_MIT) */

#include <mqueue.h>
#include "internal/syscall.h"

int mq_close(mqd_t mqd)
{
	return syscall(SYS_close, mqd);
}
