/*
 * Syscall handler
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include "syscall.h"

#include <sos.h>
#include <cspace/cspace.h>
#include <utils/util.h>
#include <serial/serial.h>

#include "proc.h"

/*
 * A dummy starting syscall
 */
#define SOS_SYSCALL0 0
#define SOS_SYSCALL_WRITE 1
#define SOS_SYSCALL_BRK 2

void
handle_syscall(struct serial *serial_port, seL4_Word badge, size_t nwords)
{
    seL4_Word syscall_number;
    seL4_CPtr reply_cap;

    seL4_MessageInfo_t reply;

    syscall_number = seL4_GetMR(0);
    void *message = seL4_GetIPCBuffer()->msg + 1; /* Skip over syscall word */

    /* Save the caller */
    reply_cap = cspace_save_reply_cap(cur_cspace);
    assert(reply_cap != CSPACE_NULL);

    switch (syscall_number) {
        case SOS_SYS_TIME:
            LOG_INFO("syscall: thread made sos_time_stamp");
            
            reply = seL4_MessageInfo_new(0, 0, 0, 1);
            timestamp_t ts = time_stamp(void);
            seL4_SetMR(0, ts >> 32);
            seL4_SetMR(1, ts & 0xFFFF);
            seL4_Send(reply_cap, reply);
            
            break;

        case SOS_SYSCALL_WRITE:
            LOG_INFO("syscall: thread made sos_write");

            size_t max_msg_size = (seL4_MsgMaxLength - 2) * sizeof(seL4_Word);
            size_t nbytes = seL4_GetMR(1);

            if (nbytes > max_msg_size)
                nbytes = max_msg_size;

            /* 
             * Byte string of characters, 4 characters in one word 
             * Skip over the nbytes field 
             */
            char *buffer = (char *)(message + sizeof(seL4_Word));

            /* Send to serial and reply with how many bytes were sent */
            nbytes = serial_send(serial_port, buffer, nbytes);
            reply = seL4_MessageInfo_new(0, 0, 0, 1);
            seL4_SetMR(0, nbytes);
            seL4_Send(reply_cap, reply);

            break;

        case SOS_SYSCALL_BRK:
            LOG_INFO("syscall: thread made sos_brk");
            
            seL4_Word newbrk = seL4_GetMR(1);
            seL4_Word *heap_b = &curproc->p_addrspace->region_heap->vaddr_start;
            seL4_Word *heap_t = &curproc->p_addrspace->region_heap->vaddr_end;

            /* will return status code and addr */
            reply = seL4_MessageInfo_new(0, 0, 0, 2);

            /* set return value as okay by default */
            seL4_SetMR(0, 0);
            
            /* if we actually desire to change heap brk */
            if (newbrk) {
                /* if the newbrk is silly, then we change return value */
                if (*heap_b > newbrk)
                    seL4_SetMR(0, 1);
                /* otherwise we change the brk */
                else 
                    *heap_t = newbrk;
            }
            
            seL4_SetMR(1, *heap_t);
            seL4_Send(reply_cap, reply);

            break;

        default:
            /* we don't want to reply to an unknown syscall */
            LOG_INFO("Unknown syscall %d", syscall_number);
    }

    /* Free the saved reply cap */
    cspace_free_slot(cur_cspace, reply_cap);
}
