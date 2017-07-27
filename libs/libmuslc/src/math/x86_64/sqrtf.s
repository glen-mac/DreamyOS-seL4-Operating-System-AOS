/* @LICENSE(MUSLC_MIT) */

.global sqrtf
.type sqrtf,@function
sqrtf:  sqrtss %xmm0, %xmm0
	ret
