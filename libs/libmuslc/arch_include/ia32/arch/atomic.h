/* @LICENSE(MUSLC_MIT) */

#ifndef _INTERNAL_ATOMIC_H
#define _INTERNAL_ATOMIC_H

#include <stdint.h>

static inline int a_ctz_64(uint64_t x)
{
    /* Compile time check that our call to the builtin will be typesafe. */
    typedef char _ull_8_bytes[sizeof(uint64_t) == sizeof(unsigned long long) ? 1 : -1];
    return __builtin_ctzll(x);
}

static inline int a_ctz_l(unsigned long x)
{
    return __builtin_ctzl(x);
}

static inline void a_and_64(volatile uint64_t *p, uint64_t v)
{
    (void)__sync_fetch_and_and(p, v);
}

static inline void a_or_64(volatile uint64_t *p, uint64_t v)
{
    (void)__sync_fetch_and_or(p, v);
}

static inline void a_store_l(volatile void *p, long x)
{
	__asm__( "movl %1, %0" : "=m"(*(long *)p) : "r"(x) : "memory" );
}

static inline void a_or_l(volatile void *p, long v)
{
    (void)__sync_fetch_and_or((volatile long*)p, v);
}

static inline void *a_cas_p(volatile void *p, void *t, void *s)
{
    return (void*)__sync_val_compare_and_swap((volatile uintptr_t*)p, (uintptr_t)t, (uintptr_t)s);
}

static inline long a_cas_l(volatile void *p, long t, long s)
{
    return __sync_val_compare_and_swap((volatile long*)p, t, s);
}

static inline int a_cas(volatile int *p, int t, int s)
{
    return __sync_val_compare_and_swap(p, t, s);
}

static inline void *a_swap_p(void *volatile *x, void *v)
{
	__asm__( "xchg %0, %1" : "=r"(v), "=m"(*(void **)x) : "0"(v) : "memory" );
	return v;
}
static inline long a_swap_l(volatile void *x, long v)
{
	__asm__( "xchg %0, %1" : "=r"(v), "=m"(*(long *)x) : "0"(v) : "memory" );
	return v;
}

static inline void a_or(volatile void *p, int v)
{
    (void)__sync_fetch_and_or((volatile int*)p, v);
}

static inline void a_and(volatile void *p, int v)
{
    (void)__sync_fetch_and_and((volatile int*)p, v);
}

static inline int a_swap(volatile int *x, int v)
{
	__asm__( "xchg %0, %1" : "=r"(v), "=m"(*x) : "0"(v) : "memory" );
	return v;
}

#define a_xchg a_swap

static inline int a_fetch_add(volatile int *x, int v)
{
    return __sync_fetch_and_add(x, v);
}

static inline int a_inc(volatile int *x)
{
    return a_fetch_add(x, 1);
}

static inline int a_dec(volatile int *x)
{
    return __sync_fetch_and_sub(x, 1);
}

static inline void a_store(volatile int *p, int x)
{
	__asm__( "movl %1, %0" : "=m"(*p) : "r"(x) : "memory" );
}

static inline void a_spin()
{
	__asm__ __volatile__( "pause" : : : "memory" );
}

static inline void a_crash()
{
	__asm__ __volatile__( "hlt" : : : "memory" );
}


#endif
