/* @LICENSE(MUSLC_MIT) */

/* These functions will not work, but suffice for targets where the
 * kernel sigaction structure does not actually use sa_restorer. */

#include "internal/syscall.h"

/*uses the syscall*/
void __restore()
{
    syscall_cp(SYS_sigreturn, 0);
}

void __restore_rt()
{
    syscall_cp(SYS_rt_sigreturn);
}
