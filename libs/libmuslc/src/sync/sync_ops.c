/* @LICENSE(NICTA) */

/* Clagged from libsel4c. */

#include <stdint.h>
#include <stdbool.h>

#if defined(ARMV6) 
/*
 * GCC is broken and doesn't provide __sync_* builtins on ARM11
 * (even with -mcpu=arm1136jf-s), despite the existence of
 * LDREX on this cpu. Here's a custom implementation.
 * 
 * Depends on tail-recursion not to stack-overflow.
 */

uint32_t __sync_fetch_and_add_4(void* location, uint32_t toadd) {
	uint32_t failure;
	uint32_t ret;
	uint32_t temp; //ask the compiler for a spare register
	asm volatile (
		"ldrex %[retv], [%[addr]]           \n"
		"add   %[temp],  %[retv] ,  %[addv] \n"
		"strex %[fail],  %[temp] , [%[addr]]\n"
		: [retv] "=&r" (ret),      /* outputs */
		  [temp] "=&r" (temp),
		  [fail]  "=r" (failure)
		: [addr]   "r" (location), /* inputs */
		  [addv]   "r" (toadd)
		: "memory"                 /* clobbers */
	);
	if(!failure)
		return ret;
	return __sync_fetch_and_add_4(location, toadd);
}

uint32_t __sync_fetch_and_or_4(void* location, uint32_t mask) {
	uint32_t failure;
	uint32_t ret;
	uint32_t temp;
	asm volatile (
		"ldrex %[retv], [%[addr]]           \n"
		"orr   %[temp],  %[retv] ,  %[mask] \n"
		"strex %[fail],  %[temp] , [%[addr]]\n"
		: [retv] "=&r" (ret),      /* outputs */
		  [temp] "=&r" (temp),
		  [fail]  "=r" (failure)
		: [addr]   "r" (location), /* inputs */
		  [mask]   "r" (mask)
		: "memory"                 /* clobbers */
	);
	if(!failure)
		return ret;
	return __sync_fetch_and_or_4(location, mask);
}

/* ihor */
uint32_t __sync_fetch_and_xor_4(void* location, uint32_t mask) {
        uint32_t failure;
        uint32_t ret;
        uint32_t temp;
        asm volatile (
                "ldrex %[retv], [%[addr]]           \n"
                "eor   %[temp],  %[retv] ,  %[mask] \n"
                "strex %[fail],  %[temp] , [%[addr]]\n"
                : [retv] "=&r" (ret),      /* outputs */
                  [temp] "=&r" (temp),
                  [fail]  "=r" (failure)
                : [addr]   "r" (location), /* inputs */
                  [mask]   "r" (mask)
                : "memory"                 /* clobbers */
        );
        if(!failure)
                return ret;
        return __sync_fetch_and_xor_4(location, mask);
}


uint32_t __sync_or_and_fetch_4(void* location, uint32_t mask) {
	uint32_t failure;
	uint32_t ret;
	asm volatile (
		"ldrex %[retv], [%[addr]]           \n"
		"orr   %[retv],  %[retv] ,  %[mask] \n"
		"strex %[fail],  %[retv] , [%[addr]]\n"
		: [retv] "=&r" (ret),      /* outputs */
		  [fail]  "=r" (failure)
		: [addr]   "r" (location), /* inputs */
		  [mask]   "r" (mask)
		: "memory"                 /* clobbers */
	);
	if(!failure)
		return ret;
	return __sync_or_and_fetch_4(location, mask);
}

/* ihor */
uint32_t __sync_sub_and_fetch_4(void* location, uint32_t mask) {
        uint32_t failure;
        uint32_t ret;
        asm volatile (
                "ldrex %[retv], [%[addr]]           \n"
                "sub   %[retv],  %[retv] ,  %[mask] \n"
                "strex %[fail],  %[retv] , [%[addr]]\n"
                : [retv] "=&r" (ret),      /* outputs */
                  [fail]  "=r" (failure)
                : [addr]   "r" (location), /* inputs */
                  [mask]   "r" (mask)
                : "memory"                 /* clobbers */
        );
        if(!failure)
                return ret;
        return __sync_sub_and_fetch_4(location, mask);
}


uint32_t __sync_fetch_and_and_4(void* location, uint32_t mask) {
	uint32_t failure;
	uint32_t ret;
	uint32_t temp;
	asm volatile (
		"ldrex %[retv], [%[addr]]           \n"
		"and   %[temp],  %[retv] ,  %[mask] \n"
		"strex %[fail],  %[temp] , [%[addr]]\n"
		: [retv] "=&r" (ret),      /* outputs */
		  [temp] "=&r" (temp),
		  [fail]  "=r" (failure)
		: [addr]   "r" (location), /* inputs */
		  [mask]   "r" (mask)
		: "memory"                 /* clobbers */
	);
	if(!failure)
		return ret;
	return __sync_fetch_and_and_4(location, mask);
}

uint32_t __sync_and_and_fetch_4(void* location, uint32_t mask) {
	uint32_t failure;
	uint32_t ret;
	asm volatile (
		"ldrex %[retv], [%[addr]]           \n"
		"and   %[retv],  %[retv] ,  %[mask] \n"
		"strex %[fail],  %[retv] , [%[addr]]\n"
		: [retv] "=&r" (ret),      /* outputs */
		  [fail]  "=r" (failure)
		: [addr]   "r" (location), /* inputs */
		  [mask]   "r" (mask)
		: "memory"                 /* clobbers */
	);
	if(!failure)
		return ret;
	return __sync_and_and_fetch_4(location, mask);
}

bool __sync_bool_compare_and_swap_4(void* location, uint32_t oldvalue, uint32_t newvalue) {
	uint32_t failure;
	uint32_t temp; //ask the compiler for a spare register
	asm volatile (
		"ldrex   %[temp], [%[addr]]           \n"
		"cmp     %[temp],  %[oval]            \n"
		"movne   %[fail],  $1                 \n"
		"strexeq %[fail],  %[nval] , [%[addr]]\n"
		: [fail] "=&r" (failure),   /* outputs */
		  [temp] "=&r" (temp)
		: [addr]   "r" (location),  /* inputs */
		  [oval]   "r" (oldvalue),
		  [nval]   "r" (newvalue)
		: "memory",                 /* clobbers */
		  "cc"
	);
	return !failure;
}

/* ihor */
void __sync_synchronize(void) {
	// asm volatile( "dmb;");
	/* ARMv6 doesn't support dmb instruction, so use cp15 */
	asm volatile("mcr p15, 0, %0, c7, c10, 5" : : "r"(0) : "memory");
}

#endif

