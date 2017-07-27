/* @LICENSE(MUSLC_MIT) */

#include <dirent.h>
#include "__dirent.h"

int dirfd(DIR *d)
{
	return d->fd;
}
