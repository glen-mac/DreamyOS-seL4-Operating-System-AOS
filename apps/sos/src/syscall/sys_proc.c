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

static int proc_delete_async_check(proc *curproc, pid_t victim_pid);

int
syscall_proc_create(proc *curproc)
{
    LOG_INFO("proc %d made syscall_proc_create", curproc->pid);
    int result = -1;

    seL4_Word name = seL4_GetMR(1);
    char kname[NAME_MAX];
    if (copy_in(curproc, kname, name, NAME_MAX) != 0) {
        LOG_ERROR("Error copying in path name");
        goto message_reply;
    }

    /* Explicit null terminate in case one is not provided */
    kname[NAME_MAX - 1] = '\0';

    // TODO: Hard copy the filename because it might cross a page boundary
    // name = vaddr_to_sos_vaddr(curproc, name, ACCESS_READ);

    result = proc_start(kname, _sos_ipc_ep_cap, curproc->pid);
    message_reply:
        seL4_SetMR(0, result);
        return 1; /* nwords in message */
}

int
syscall_proc_delete(proc *curproc)
{
    pid_t victim_pid = seL4_GetMR(1);
    LOG_INFO("Proc %d made proc_delete(%d)", curproc->pid, victim_pid);
    int ret_val = proc_delete_async_check(curproc, victim_pid);
    return ret_val;
}

int
syscall_proc_id(proc *curproc)
{
    LOG_INFO("syscall proc_id: PID %d", curproc->pid);
    seL4_SetMR(0, (seL4_Word)curproc->pid);
    return 1; 
}

int
syscall_proc_status(proc *curproc)
{
    // TODO: currently might write over page boundary

    LOG_INFO("syscall proc_status: PID %d", curproc->pid);
    seL4_Word sos_procs_addr = seL4_GetMR(1);
    seL4_Word procs_max = seL4_GetMR(2);

    sos_process_t *sos_procs = (sos_process_t *)vaddr_to_sos_vaddr(curproc, sos_procs_addr, ACCESS_READ);
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
        sos_procs->size = page_directory_count(c_proc);
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

int
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
                goto destroy;
            }
        }
        /* Need to wait on a child */
        goto wait;
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

    wait:
        curproc->waiting_coro = coro_getcur();
        curproc->waiting_on = pid;
        ret_val = yield(NULL);
        curproc->waiting_coro = NULL;
        curproc->waiting_on = -1;

    destroy:
        proc_destroy(get_proc(ret_val));

    message_reply:
        seL4_SetMR(0, ret_val);
        return 1; /* Number of words to return */
}

int
syscall_exit(proc *curproc)
{
    LOG_INFO("proc %d called exit", curproc->pid);
    return proc_delete_async_check(curproc, curproc->pid);
}

int
proc_delete_async_check(proc *curproc, pid_t victim_pid)
{
    int ret_val = -1;
    bool no_reply = FALSE;
    proc *victim = get_proc(victim_pid);
    if (!victim) {
        LOG_ERROR("Invalid process");
        goto message_reply;
    }

    if (victim->p_state == BLOCKED) {
        /* 
         * If the proc is blocked, we mark it for death
         * It will be deleted when it becomes unblocked
         */
        LOG_INFO("Process is blocked");
        victim->kill_flag = TRUE;
        ret_val = victim->pid;
        goto message_reply;
    }

    if (proc_delete(victim) != 0) {
        LOG_ERROR("Failed to delete process");
        goto message_reply;
    }

    ret_val = victim->pid;
    if (curproc->pid == victim_pid)
        no_reply = TRUE;

    message_reply:
        LOG_INFO("SETTING RETVAL %d", ret_val);
        seL4_SetMR(0, (seL4_Word)ret_val);
        /* We dont want to reply if we destroyed ourselves, as the cap will be invalid */
        if (no_reply)
            return -1;

        return 1; /* nwords in message */
}
