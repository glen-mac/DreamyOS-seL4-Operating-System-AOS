/* @LICENSE(MUSLC_MIT) */

#include <strings.h>
#include <ctype.h>

int strcasecmp_l(const char *l, const char *r, locale_t loc)
{
	return strcasecmp(l, r);
}
