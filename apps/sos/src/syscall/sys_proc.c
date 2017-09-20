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
    LOG_INFO("syscall proc_id: PID %d", curproc->pid);
    seL4_SetMR(0, (seL4_Word)curproc->pid);
    return 1; 
}

seL4_Word
syscall_proc_status(proc *curproc)
{
    //TODO: currently might write over page boundary 
    
    LOG_INFO("syscall proc_status: PID %d", curproc->pid);

    sos_process_t sos_procs = (sos_process_t)vaddr_to_sos_vaddr(curproc, seL4_GetMR(1), ACCESS_READ);
    seL4_Word procs_max = seL4_GetMR(2);
    seL4_Word num_found = 0;
    pid_t c_id = 0;
    proc *c_proc;
    
    /* loop over all procs */
    while (num_found < procs_max || c_id != MAX_PROCS) {
        /* if there is no proc then skip over */
        if ((c_proc = get_proc(c_id)) == NULL) {
            goto proc_loop;
        }
        /* write proc info */
        num_found++;
        sos_procs->pid = c_proc->pid;
        sos_procs->size = 1337;
        sos_procs->stime = c_proc->stime;
        sos_procs->command = c_proc->proc_name;
        strcpy(sos_procs->command, c_proc->proc_name);  
        /* get next sos procs struct over in buffer */
        sos_procs = sos_procs + 1;
proc_loop:
        c_id++;
    }

    seL4_SetMR(0, (seL4_Word)num_found);
    return 1; 
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
    assert(seL4_TCB_Suspend(curproc->tcb_cap) == 0);
    LOG_INFO("after");
    return 0;
}
