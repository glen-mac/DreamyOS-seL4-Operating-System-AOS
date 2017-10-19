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

    seL4_Word heap_s = curproc->p_addrspace->region_heap->start;
    seL4_Word *heap_e = &(curproc->p_addrspace->region_heap->end);

    /* If we actually desire to change heap brk */
    if (newbrk) {
        if (as_region_collision_check(curproc->p_addrspace, curproc->p_addrspace->region_heap, heap_s, newbrk) != 0) {
            LOG_ERROR("Heap extension failed collision check");
            goto brk_epilogue;
        }

        *heap_e = newbrk;
    }

    brk_epilogue:
        LOG_INFO("returning %p", *heap_e);
        seL4_SetMR(0, *heap_e);
        return 1;
}
