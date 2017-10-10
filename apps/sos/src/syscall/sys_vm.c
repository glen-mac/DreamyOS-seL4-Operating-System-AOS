/*
 * VM Syscall handler
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include "sys_vm.h"

#include <proc/proc.h>
#include <utils/util.h>

int
syscall_brk(proc *curproc)
{
    intptr_t newbrk = seL4_GetMR(1);
    LOG_INFO("syscall: thread made sos_brk(%d)", newbrk);

    intptr_t *heap_s = &(curproc->p_addrspace->region_heap->start);
    intptr_t *heap_e = &(curproc->p_addrspace->region_heap->end);
    intptr_t stack_s = curproc->p_addrspace->region_stack->start;

    /* Set return value as okay by default */
    seL4_SetMR(0, 0);

    /* If we actually desire to change heap brk */
    if (newbrk) {
        if (newbrk < *heap_s) {
            /* If the newbrk is silly, then we change return value */
            seL4_SetMR(0, 1);
        } else if (newbrk >= stack_s){
            /* If the newbrk is actually WITHIN the DEFINED stack region */
            seL4_SetMR(0, 1);
        } else {
            /* otherwise we change the brk */
            *heap_e = newbrk;
        }
    }

    seL4_SetMR(1, *heap_e);
    return 2;
}
