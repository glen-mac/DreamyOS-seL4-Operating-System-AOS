/* @LICENSE(MUSLC_MIT) */

#include "stdio_impl.h"

int getchar_unlocked(void)
{
	return getc_unlocked(stdin);
}
