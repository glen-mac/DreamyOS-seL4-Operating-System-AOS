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
    int ret_val;    

    LOG_INFO("Proc %d made proc_delete(%d)", curproc->pid, victim_pid);

    if ((ret_val = proc_delete_async_check(curproc, victim_pid)) != -1)
        ret_val = 0;

    /* No reply if process deleted itself */
    if (curproc->pid == victim_pid)
        return -1;

    seL4_SetMR(0, (seL4_Word)ret_val);
    return 1;
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
    LOG_INFO("syscall proc_status: PID %d", curproc->pid);
    seL4_Word sos_procs_addr = seL4_GetMR(1);
    seL4_Word procs_max = seL4_GetMR(2);

    // sos_process_t *sos_procs = (sos_process_t *)vaddr_to_sos_vaddr(curproc, sos_procs_addr, ACCESS_READ);
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
        sos_process_t kproc = {
            .pid = c_proc->pid,
            .size = page_directory_count(c_proc),
            .stime = c_proc->stime,
        };
        strcpy(kproc.command, c_proc->proc_name);

        /* Copy out stat to user process */
        if (copy_out(curproc, sos_procs_addr, &kproc, sizeof(sos_process_t)) != 0) {
            LOG_ERROR("Error copying out");
            goto message_reply;
        }

        /* get next sos procs struct over in buffer */
        sos_procs_addr += sizeof(sos_process_t);
        num_found++;

        proc_loop:
            c_id++;
    }

    message_reply:
        seL4_SetMR(0, num_found);
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
                LOG_INFO("child pid is %d", ret_val);
                goto destroy;
            }
        }
        /* Need to wait on a child */
        goto wait;
    }

    if (!proc_is_child(curproc, pid)) {
        LOG_ERROR("%d is not the calling procs child", pid);
        proc *p = get_proc(pid);
        if (p)
            LOG_ERROR("procs parent is %d", p->ppid);
        goto message_reply;
    }

    proc *child = get_proc(pid);
    assert(child != NULL);

    if (child->p_state == ZOMBIE) {
        LOG_INFO("Child already exited");
        ret_val = pid;
        LOG_INFO("pid is %d", ret_val);
        goto destroy;
    }

    wait:
        curproc->waiting_coro = coro_getcur();
        curproc->waiting_on = pid;
        ret_val = yield(NULL);
        curproc->waiting_coro = NULL;
        curproc->waiting_on = -1;

    LOG_INFO("yieled and got this %d", ret_val);

    destroy:
        LOG_INFO("ret_val is %d", ret_val);
        proc_destroy(get_proc(ret_val));

    message_reply:
        seL4_SetMR(0, ret_val);
        return 1; /* Number of words to return */
}

int
syscall_exit(proc *curproc)
{
    LOG_INFO("proc %d called exit", curproc->pid);
    proc_delete_async_check(curproc, curproc->pid);
    /* No reply to self as we have exited */
    return -1;
}

int
proc_delete_async_check(proc *curproc, pid_t victim_pid)
{
    int ret_val = -1;
    proc *victim = get_proc(victim_pid);
    if (!victim) {
        LOG_ERROR("Invalid process");
        return ret_val;
    }

    if (victim->p_state == BLOCKED) {
        /* 
         * If the proc is blocked, we mark it for death
         * It will be deleted when it becomes unblocked
         */
        LOG_INFO("Process is blocked");
        victim->kill_flag = TRUE;
        ret_val = 0;
        return ret_val;
    }

    if (proc_delete(victim) != 0) {
        LOG_ERROR("Failed to delete process");
        return ret_val;
    }

    ret_val = 0;
    return ret_val;
}
