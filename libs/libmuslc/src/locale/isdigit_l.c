/* @LICENSE(MUSLC_MIT) */

#include <ctype.h>

int isdigit_l(int c, locale_t l)
{
	return isdigit(c);
}
