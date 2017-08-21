/*
 * VM Syscall handler
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include "syscall.h"
#include "sys_vm.h"
#include "proc.h"
#include <sos.h>
#include <cspace/cspace.h>
#include <utils/util.h>

void
syscall_brk(seL4_CPtr reply_cap) {
    seL4_MessageInfo_t reply;
    
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
}
