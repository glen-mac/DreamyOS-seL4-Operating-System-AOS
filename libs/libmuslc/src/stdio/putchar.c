/* @LICENSE(MUSLC_MIT) */

#include <stdio.h>

int putchar(int c)
{
	return fputc(c, stdout);
}
