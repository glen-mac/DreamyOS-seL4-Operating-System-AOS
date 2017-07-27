/* @LICENSE(MUSLC_MIT) */

#include <stdlib.h>
#include "internal/syscall.h"

void _Exit(int ec)
{

    __syscall(SYS_exit_group, ec);
    __syscall(SYS_exit, ec);
}
