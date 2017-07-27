/* @LICENSE(MUSLC_MIT) */

static inline struct pthread *__pthread_self()
{
	struct pthread *self;
#ifdef CONFIG_X86_64
    __asm__ __volatile__ ("movq %%gs:0,%0" : "=r" (self) );
#else
	__asm__ __volatile__ ("movl %%gs:0,%0" : "=r" (self) );
#endif
	return self;
}

#define CANCEL_REG_SP 7
#define CANCEL_REG_IP 14
