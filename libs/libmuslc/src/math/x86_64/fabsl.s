/* @LICENSE(MUSLC_MIT) */

.global fabsl
.type fabsl,@function
fabsl:
	fldt 8(%rsp)
	fabs
	ret
