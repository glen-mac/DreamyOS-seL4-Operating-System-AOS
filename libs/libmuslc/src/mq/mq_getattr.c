/* @LICENSE(MUSLC_MIT) */

#include <mqueue.h>
#include "internal/syscall.h"

int mq_getattr(mqd_t mqd, struct mq_attr *attr)
{
	return mq_setattr(mqd, 0, attr);
}
