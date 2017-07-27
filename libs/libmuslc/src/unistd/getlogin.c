/* @LICENSE(MUSLC_MIT) */

#include <unistd.h>
#include <stdlib.h>

char *getlogin(void)
{
	return getenv("LOGNAME");
}
