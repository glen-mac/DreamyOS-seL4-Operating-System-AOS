/*
 * Process Syscalls
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include "sys_proc.h"

#include "event.h" /* Only for CPIO archive and _sos_ipc_ep_cap */

#include <proc/proc.h>
#include <vm/vm.h>
#include <sys/panic.h>
#include <utils/util.h>
#include <utils/list.h>

static seL4_Word do_proc_delete(pid_t pid);

seL4_Word
syscall_proc_create(proc *curproc)
{
    LOG_INFO("proc %d made syscall_proc_create", curproc->pid);

    // TODO: Hard copy the filename because it might cross a page boundary
    seL4_Word name = vaddr_to_sos_vaddr(curproc, seL4_GetMR(1), ACCESS_READ);

    pid_t pid = proc_start(_cpio_archive, name, _sos_ipc_ep_cap, curproc->pid);
    if (pid == -1) {
        LOG_ERROR("Error starting process");
        goto message_reply;
    }

    if (list_prepend(curproc->children, pid) != 0) {
        LOG_ERROR("Error creating child");
        pid = -1;
    }

    message_reply:
        seL4_SetMR(0, (seL4_Word)pid);
        return 1; /* nwords in message */
}

seL4_Word
syscall_proc_delete(proc *curproc)
{
    pid_t pid = seL4_GetMR(1);
    LOG_INFO("Proc %d made proc_delete(%d)", curproc->pid, pid);
    return do_proc_delete(pid);
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
    sos_process_t * sos_procs = (sos_process_t *)vaddr_to_sos_vaddr(curproc, seL4_GetMR(1), ACCESS_READ);
    seL4_Word procs_max = seL4_GetMR(2);
    seL4_Word num_found = 0;
    pid_t c_id = 0;
    proc *c_proc;

    /* loop over all procs */
    while (num_found < procs_max && c_id < MAX_PROCS) {
        /* if there is no proc then skip over */
        if ((c_proc = get_proc(c_id)) == NULL) {
            goto proc_loop;
        }
        /* write proc info */
        num_found++;
        sos_procs->pid = c_proc->pid;
        sos_procs->size = page_directory_count(curproc);
        sos_procs->stime = c_proc->stime;
        strcpy(sos_procs->command, c_proc->proc_name);  
        /* get next sos procs struct over in buffer */
        sos_procs++;
        proc_loop:
            c_id++;
    }
    seL4_SetMR(0, (seL4_Word)num_found);
    return 1;
}

seL4_Word
syscall_proc_wait(proc *curproc)
{
    pid_t pid = seL4_GetMR(1);
    int ret_val = -1;

    LOG_INFO("%d called proc_wait(%d)", curproc->pid, pid);

    /* Check all children if called with -1 */
    if (pid == -1) {
        for (struct list_node *curr = curproc->children->head; curr != NULL; curr = curr->next) {
            proc *child = get_proc(curr->data);
            if (child && child->p_state == ZOMBIE) {
                ret_val = child->pid;
                goto message_reply;
            }
        }
    }

    if (!proc_is_child(curproc, pid)) {
        LOG_ERROR("%d is not the calling procs child", pid);
        goto message_reply;
    }

    proc *child = get_proc(pid);
    if (child->p_state == ZOMBIE) {
        LOG_INFO("Child already exited");
        ret_val = pid;
        goto destroy;
    }

    curproc->waiting_coro = coro_getcur();
    curproc->waiting_on = pid;
    ret_val = yield(NULL);
    curproc->waiting_coro = NULL;
    curproc->waiting_on = -1;

    destroy:
        proc_destroy(child);

    message_reply:
        seL4_SetMR(0, ret_val);
        return 1; /* Number of words to return */
}

seL4_Word
syscall_exit(proc *curproc)
{
    LOG_INFO("proc %d called exit", curproc->pid);
    return do_proc_delete(curproc->pid);
}

static seL4_Word
do_proc_delete(pid_t pid)
{
    proc *victim = get_proc(pid);
    int ret_val = -1;

    // TODO: Handle async requests

    if (victim->p_state != RUNNING) {
        LOG_ERROR("Can only delete running processes");
        goto message_reply;
    }
    victim->p_state = ZOMBIE;

    /* Stop the thread */
    if (seL4_TCB_Suspend(victim->tcb_cap) != 0) {
        LOG_ERROR("Failed to suspend thread");
        goto message_reply;
    }

    // TODO Destroy TCB???
    victim->tcb_cap = NULL;

    /* IPC destroy */
    if (seL4_ARM_Page_Unmap(victim->ipc_buffer_cap) != 0) {
        LOG_ERROR("Failed to unmap IPC buffer");
        goto message_reply;
    }

    if (cspace_delete_cap(cur_cspace, victim->ipc_buffer_cap) != 0) {
        LOG_ERROR("Failed to delete IPC buffer cap");
        goto message_reply;
    }
    victim->ipc_buffer_cap = NULL;

    /* Destroy the Cspace */
    if (cspace_destroy(victim->croot) != CSPACE_NOERROR) {
        LOG_ERROR("Failed to destroy cspace");
        goto message_reply;
    }
    victim->croot = NULL;

    /* Destroy the address space */
    if (as_destroy(victim->p_addrspace) != 0) {
        LOG_ERROR("Failed to destroy addrspace");
        goto message_reply;
    }
    victim->p_addrspace = NULL;

    /* Destroy the file table */
    if (fdtable_destroy(victim->file_table) != 0) {
        LOG_ERROR("Failed to destroy file table");
        goto message_reply;
    }

    victim->file_table = NULL;

    /* victim is not running but the struct is still needed for wait pid */
    ret_val = victim->pid;

    proc *parent = get_proc(victim->ppid);
    if (!parent) {
        LOG_ERROR("Unable to find parent");
        ret_val = -1;
    }

    if (proc_is_waiting(parent, victim)) {
        LOG_INFO("resuming coroutine");
        resume(parent->waiting_coro, victim->pid);
    }

    message_reply:
        seL4_SetMR(0, (seL4_Word)ret_val);
        return 1; /* nwords in message */
}
