/*
 * Syscall handler
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include "syscall.h"
#include <cspace/cspace.h>
#include <utils/util.h>

/* include all sys_* wrappers */
#include "sys_time.h"
#include "sys_file.h"
#include "sys_vm.h"
#include "sys_proc.h"
#include "event.h"

/* Currently dependent on syscall numbers ordering, might change this */
static seL4_Word (*syscall_table[])(void) = {
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
    syscall_time_stamp,
    syscall_stat,
    syscall_listdir,
    syscall_proc_create,
    syscall_proc_delete,
    syscall_proc_id,
    syscall_proc_status,
    syscall_proc_wait,
};

void
handle_syscall(seL4_Word badge)
{
    seL4_Word syscall_number = seL4_GetMR(0);
    pid_t cur_proc = GET_PROCID_BADGE(badge);

    /* Save the caller */
    seL4_CPtr reply_cap = cspace_save_reply_cap(cur_cspace);
    assert(reply_cap != CSPACE_NULL);

    /* If syscall number is valid and function pointer is not NULL */
    if (ISINRANGE(0, syscall_number, ARRAY_SIZE(syscall_table) - 1) &&
        syscall_table[syscall_number]) {
        seL4_MessageInfo_t reply = seL4_MessageInfo_new(0, 0, 0, syscall_table[syscall_number]());
        seL4_Send(reply_cap, reply);
    } else {
        LOG_INFO("Unknown syscall %d", syscall_number);
    }

    /* Free the saved reply cap */
    cspace_free_slot(cur_cspace, reply_cap);
}
