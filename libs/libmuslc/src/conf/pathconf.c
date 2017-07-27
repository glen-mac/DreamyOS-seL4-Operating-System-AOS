/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>

long pathconf(const char *path, int name)
{
	return fpathconf(-1, name);
}
