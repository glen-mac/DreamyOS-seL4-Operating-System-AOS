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
    seL4_Word newbrk = seL4_GetMR(1);

    LOG_SYSCALL(curproc->pid, "brk(%p)", newbrk);

    seL4_Word *heap_s = &(curproc->p_addrspace->region_heap->start);
    seL4_Word *heap_e = &(curproc->p_addrspace->region_heap->end);
    seL4_Word stack_s = curproc->p_addrspace->region_stack->start;

    /* If we actually desire to change heap brk */
    if (newbrk) {
        if (newbrk < *heap_s) {
            /* If the newbrk is silly, then we change return value */
            seL4_SetMR(0, 1);
        } else if (newbrk >= stack_s) {
            /* If the newbrk is actually WITHIN the DEFINED stack region */
            seL4_SetMR(0, 1);
        } else {
            /* otherwise we change the brk */
            *heap_e = newbrk;
        }
    }

    seL4_SetMR(0, *heap_e);
    return 1;
}
