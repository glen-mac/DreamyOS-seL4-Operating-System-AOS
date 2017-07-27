/* @LICENSE(MUSLC_MIT) */

#include "libm.h"

double complex conj(double complex z)
{
	return cpack(creal(z), -cimag(z));
}
