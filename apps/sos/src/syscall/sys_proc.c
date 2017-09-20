/*
 * Process Syscalls
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include "sys_proc.h"

#include "event.h" /* Only for CPIO archive and _sos_ipc_ep_cap */

#include <proc/proc.h>
#include <sys/panic.h>
#include <utils/util.h>


seL4_Word
syscall_proc_create(proc *curproc)
{
    LOG_INFO("proc %d made syscall_proc_create", curproc->pid);

    // TODO: Hard copy the filename because it might cross a page boundary
    seL4_Word name = vaddr_to_sos_vaddr(curproc, seL4_GetMR(1), ACCESS_READ);

    pid_t pid = proc_start(_cpio_archive, name, _sos_ipc_ep_cap);
    if (pid == -1)
        LOG_ERROR("Error starting process");

    seL4_SetMR(0, (seL4_Word)pid);
    return 1; /* nwords in message */
}

seL4_Word
syscall_proc_delete(proc *curproc)
{
    LOG_INFO("Proc %d made syscall_proc_delete", curproc->pid);

    // TODO CHECK STATUS OF PROC, SHOULD BE RUNNNING

    int ret_val = -1;

    /* Stop the thread */
    if (seL4_TCB_Suspend(curproc->tcb_cap) != 0) {
        LOG_ERROR("Failed to suspend thread");
        goto message_reply;
    }

    // TODO Destroy TCB???
    curproc->tcb_cap = NULL;

    /* IPC destroy */
    if (seL4_ARM_Page_Unmap(curproc->ipc_buffer_cap) != 0) {
        LOG_ERROR("Failed to unmap IPC buffer");
        goto message_reply;
    }

    if (cspace_delete_cap(cur_cspace, curproc->ipc_buffer_cap) != 0) {
        LOG_ERROR("Failed to delete IPC buffer cap");
        goto message_reply;
    }

    /* Destroy the Cspace */
    if (cspace_destroy(curproc->croot) != CSPACE_NOERROR) {
        LOG_ERROR("Failed to destroy cspace");
        goto message_reply;
    }

    /* Destroy the address space */
    if (as_destroy(curproc->p_addrspace) != 0) {
        LOG_ERROR("Failed to destroy addrspace");
        goto message_reply;
    }

    // TODO DESTROY FILE TABLE

    /* curproc is not running but the struct is still needed for wait pid */
    ret_val = curproc->pid;

    message_reply:
        seL4_SetMR(0, (seL4_Word)ret_val);
        return 1; /* nwords in message */
}

seL4_Word
syscall_proc_id(proc *curproc)
{
    panic("not implemented");
}

seL4_Word
syscall_proc_status(proc *curproc)
{
    panic("not implemented");
}

seL4_Word
syscall_proc_wait(proc *curproc)
{
    panic("not implemented");
}

seL4_Word
syscall_exit(proc *curproc)
{
    LOG_INFO("proc %d called exit", curproc->pid);
    return syscall_proc_delete(curproc);
}
