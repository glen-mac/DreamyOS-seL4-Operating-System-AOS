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

coro syscall_coro = NULL;

void
handle_syscall(seL4_Word badge, size_t nwords)
{
    seL4_Word syscall_number;
    seL4_CPtr reply_cap;

    syscall_number = seL4_GetMR(0);

    /* Save the caller */
    reply_cap = cspace_save_reply_cap(cur_cspace);
    assert(reply_cap != CSPACE_NULL);

    // TODO: Turn this into a jump table
    switch (syscall_number) {
        case SOS_SYS_USLEEP:
            syscall_usleep(reply_cap);
            break;

        case SOS_SYS_TIME_STAMP:
            syscall_time_stamp(reply_cap);
            break;

        case SOS_SYS_OPEN:
            syscall_open(reply_cap);
            break;

        case SOS_SYS_WRITE:
            syscall_write(reply_cap);
            break;

        case SOS_SYS_READ:
            syscall_coro = coroutine(syscall_read);
            resume(syscall_coro, reply_cap); /* Run the read handler in a coroutine */
            break;

        case SOS_SYS_CLOSE:
            syscall_close(reply_cap);
            break;

        case SOS_SYS_BRK:
            syscall_brk(reply_cap);
            break;

        default:
            /* we don't want to reply to an unknown syscall */
            LOG_INFO("Unknown syscall %d", syscall_number);
    }

    /* Free the saved reply cap */
    cspace_free_slot(cur_cspace, reply_cap);
}
