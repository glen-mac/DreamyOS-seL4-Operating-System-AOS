/*
 * VM Syscall handler
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include "sys_vm.h"

#include <proc/proc.h>
#include <utils/util.h>

seL4_Word
syscall_brk(proc * curproc)
{
    seL4_Word newbrk = seL4_GetMR(1);
    LOG_INFO("syscall: thread made sos_brk(%d)", newbrk);

    seL4_Word *heap_b = &(curproc->p_addrspace->region_heap->start);
    seL4_Word *heap_t = &(curproc->p_addrspace->region_heap->end);

    /* Set return value as okay by default */
    seL4_SetMR(0, 0);

    /* If we actually desire to change heap brk */
    if (newbrk) {
        if (*heap_b > newbrk) {
            /* If the newbrk is silly, then we change return value */
            seL4_SetMR(0, 1);
        } else {
            /* otherwise we change the brk */
            *heap_t = newbrk;
        }
    }

    seL4_SetMR(1, *heap_t);
    return 2;
}
