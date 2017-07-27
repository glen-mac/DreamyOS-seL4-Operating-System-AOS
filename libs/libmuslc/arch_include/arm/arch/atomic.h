/* @LICENSE(MUSLC_MIT) */

#ifndef _INTERNAL_ATOMIC_H
#define _INTERNAL_ATOMIC_H

#include <stdint.h>
#include <stdbool.h>
static inline int a_ctz_l(unsigned long x)
{
    return __builtin_ctzl(x);
}

static inline int a_ctz_64(uint64_t x)
{
    /* Compile time check that our call to the builtin will be typesafe. */
    typedef char _ull_8_bytes[sizeof(uint64_t) == sizeof(unsigned long long) ? 1 : -1];
    return __builtin_ctzll(x);
}

static inline int a_cas(volatile int *p, int t, int s)
{
    return __sync_val_compare_and_swap(p, t, s);
}

static inline void *a_cas_p(volatile void *p, void *t, void *s)
{
	return (void *)a_cas(p, (int)t, (int)s);
}

static inline long a_cas_l(volatile void *p, long t, long s)
{
	return a_cas(p, t, s);
}

static inline int a_swap(volatile int *x, int v)
{
	int old;
	do old = *x;
	while (a_cas(x, old, v) != old);
	return old;
}

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
	*p=x;
}

static inline void a_spin()
{
}

static inline void a_crash()
{
	*(volatile char *)0=0;
}

static inline void a_and(volatile int *p, int v)
{
    (void)__sync_fetch_and_and(p, v);
}

static inline void a_or(volatile int *p, int v)
{
    (void)__sync_fetch_and_or(p, v);
}

static inline void a_and_64(volatile uint64_t *p, uint64_t v)
{
	union { uint64_t v; uint32_t r[2]; } u = { v };
	a_and((int *)p, u.r[0]);
	a_and((int *)p+1, u.r[1]);
}

static inline void a_or_64(volatile uint64_t *p, uint64_t v)
{
	union { uint64_t v; uint32_t r[2]; } u = { v };
	a_or((int *)p, u.r[0]);
	a_or((int *)p+1, u.r[1]);
}

#endif
