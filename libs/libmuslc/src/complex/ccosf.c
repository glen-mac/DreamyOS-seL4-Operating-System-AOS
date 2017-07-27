/* @LICENSE(MUSLC_MIT) */

#include "libm.h"

float complex ccosf(float complex z)
{
	return ccoshf(cpackf(-cimagf(z), crealf(z)));
}
