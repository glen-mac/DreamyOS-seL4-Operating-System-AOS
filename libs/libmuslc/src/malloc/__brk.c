/* @LICENSE(MUSLC_MIT) */

#include <stdint.h>
#include "internal/syscall.h"


uintptr_t __brk(uintptr_t newbrk)
{
    return syscall(SYS_brk, newbrk);

}
