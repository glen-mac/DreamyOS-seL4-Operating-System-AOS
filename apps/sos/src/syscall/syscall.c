/*
 * Syscall handler
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include <cspace/cspace.h>

#include "picoro.h"
#include "proc.h"
#include <sos.h>
#include "syscall.h"
#include <utils/util.h>

/* include all sys_* wrappers */
#include "sys_time.h"
#include "sys_file.h"
#include "sys_vm.h"

static void (*syscall_table[])(seL4_CPtr) = {
    NULL,
    syscall_write,
    NULL,
    syscall_read,
    NULL,
    syscall_open,
    syscall_close,
    syscall_brk,
    NULL,
    NULL,
    NULL,
    NULL,
    syscall_usleep,
    syscall_time_stamp
};

void
handle_syscall(seL4_Word badge)
{
    seL4_Word syscall_number = seL4_GetMR(0);

    /* Save the caller */
    seL4_CPtr reply_cap = cspace_save_reply_cap(cur_cspace);
    assert(reply_cap != CSPACE_NULL);

    if (ISINRANGE(0, syscall_number, ARRAY_SIZE(syscall_table)) && syscall_table[syscall_number]) {
        syscall_table[syscall_number](reply_cap);
    } else {
        LOG_INFO("Unknown syscall %d", syscall_number);
    }

    /* Delete the saved reply cap */
    cspace_delete_cap(cur_cspace, reply_cap);
}
