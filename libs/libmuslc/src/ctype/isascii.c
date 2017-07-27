/* @LICENSE(MUSLC_MIT) */

#include <ctype.h>

int isascii(int c)
{
	return !(c&~0x7f);
}
