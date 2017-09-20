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
    // TODO: Hard copy the filename because it might cross a page boundary
    LOG_INFO("syscall proc_create");

    seL4_Word name = vaddr_to_sos_vaddr(curproc, seL4_GetMR(1), ACCESS_READ);

    LOG_INFO("name is %s", name);

    pid_t pid = proc_start(_cpio_archive, name, _sos_ipc_ep_cap);
    if (pid == -1)
        LOG_ERROR("Error starting process");

    LOG_INFO("after proc_start");

    seL4_SetMR(0, (seL4_Word)pid);
    return 1; /* nwords in message */
}

seL4_Word
syscall_proc_delete(proc *curproc)
{
    panic("not implemented");
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
    LOG_INFO("tcp_cap %d", curproc->tcb_cap);
    return seL4_TCB_Suspend(curproc->tcb_cap);
}
