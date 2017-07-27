/* @LICENSE(MUSLC_MIT) */

#include "libm.h"

double complex cproj(double complex z)
{
	if (isinf(creal(z)) || isinf(cimag(z)))
		return cpack(INFINITY, copysign(0.0, creal(z)));
	return z;
}
