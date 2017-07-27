/* @LICENSE(MUSLC_MIT) */

.global rint
.type rint,@function
rint:
	fldl 4(%esp)
	frndint
	ret
