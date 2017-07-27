/* @LICENSE(MUSLC_MIT) */

#include <dirent.h>
#include "__dirent.h"

long telldir(DIR *dir)
{
	return dir->tell;
}
