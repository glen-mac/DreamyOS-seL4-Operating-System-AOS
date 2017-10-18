/*
 * Process Syscalls
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include "sys_proc.h"

#include <event.h> /* For _sos_ipc_ep_cap */
#include <proc/proc.h>
#include <vm/vm.h>
#include <utils/util.h>

static int proc_delete_async_check(pid_t victim_pid);

int
syscall_proc_create(proc *curproc)
{
    int result = -1;

    seL4_Word name = seL4_GetMR(1);

    LOG_SYSCALL(curproc->pid, "sos_process_create(%p)", name);

    /* Copy the filename into a local buffer, as the name may span multiple frames */
    char kname[NAME_MAX];
    if (copy_in(curproc, kname, name, NAME_MAX) != 0) {
        LOG_ERROR("Error copying in filename");
        goto message_reply;
    }

    /* Explicit null terminate in case one is not provided */
    kname[NAME_MAX - 1] = '\0';

    result = proc_start(kname, _sos_ipc_ep_cap, curproc->pid);

    message_reply:
        seL4_SetMR(0, result);
        return 1; /* nwords in message */
}

int
syscall_proc_delete(proc *curproc)
{
    int result = -1;

    pid_t victim_pid = seL4_GetMR(1);

    LOG_SYSCALL(curproc->pid, "sos_process_delete(%d)", victim_pid);

    /* Try to delete the process, checking if it is blocked */
    if (proc_delete_async_check(victim_pid) != 0) {
        LOG_ERROR("Failed to mark process for deletion");
        goto message_reply;
    }

    /* No reply if process deleted itself */
    if (curproc->pid == victim_pid)
        return -1;

    result = 0;
    message_reply:
        seL4_SetMR(0, (seL4_Word)result);
        return 1;
}

int
syscall_proc_id(proc *curproc)
{
    LOG_SYSCALL(curproc->pid, "sos_my_id()");
    seL4_SetMR(0, (seL4_Word)curproc->pid);
    return 1; 
}

int
syscall_proc_status(proc *curproc)
{
    seL4_Word sos_procs_addr = seL4_GetMR(1);
    seL4_Word procs_max = seL4_GetMR(2);

    LOG_SYSCALL(curproc->pid, "sos_process_status(%p, %u)", (void *)sos_procs_addr, procs_max);

    seL4_Word num_found = 0;
    pid_t c_id = 0;
    proc *c_proc = NULL;

    /* Loop over all procs */
    while (num_found < procs_max && c_id < MAX_PROCS) {
        /* Skip empty pid's */
        if ((c_proc = get_proc(c_id)) == NULL)
            goto proc_loop;

        /* Bundle process info */
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

        /* Shift to next spot in the processes struct buffer */
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
    int result = -1;

    pid_t pid = seL4_GetMR(1);

    LOG_SYSCALL(curproc->pid, "wait(%d)", pid);

    /* Check all children if called with -1 */
    if (pid == -1) {
        for (struct list_node *curr = curproc->children->head; curr != NULL; curr = curr->next) {
            proc *child = get_proc(curr->data);
            /* Destroy the child if it has exited */
            if (child && child->p_state == ZOMBIE) {
                result = child->pid;
                goto destroy;
            }
        }
        /* Need to wait on a child */
        goto wait;
    }

    /* Can only wait on your child */
    if (!proc_is_child(curproc, pid)) {
        LOG_ERROR("%d is not the calling procs child", pid);
        goto message_reply;
    }

    /* Check if child has already exited */
    proc *child = get_proc(pid);
    assert(child != NULL);
    if (child->p_state == ZOMBIE) {
        LOG_INFO("Child already exited");
        result = pid;
        goto destroy;
    }

    /* Wait for the child process to exit */
    wait:
        curproc->waiting_coro = coro_getcur();
        curproc->waiting_on = pid;
        result = yield(NULL);
        curproc->waiting_coro = NULL;
        curproc->waiting_on = -1;

    destroy:
        proc_destroy(get_proc(result));

    message_reply:
        seL4_SetMR(0, result);
        return 1; /* Number of words to return */
}

int
syscall_exit(proc *curproc)
{
    LOG_SYSCALL(curproc->pid, "exit()");
    proc_delete_async_check(curproc->pid);
    /* No reply to self as we have exited */
    return -1;
}

/*
 * Flag the process for deletetion when it is unblocked
 * If it is unblocked, immediately delete it
 * @param victim_pid, the victim pid to destroy
 * @returns 0 on success else 1
 */
static int
proc_delete_async_check(pid_t victim_pid)
{
    proc *victim = get_proc(victim_pid);
    if (!victim) {
        LOG_ERROR("Invalid process");
        return 1;
    }

    if (victim->p_state == BLOCKED) {
        /* 
         * If the proc is blocked, we mark it for death
         * It will be deleted when it becomes unblocked
         */
        LOG_INFO("Process is blocked");
        victim->kill_flag = TRUE;
        return 0;
    }

    if (proc_delete(victim) != 0) {
        LOG_ERROR("Failed to delete process");
        return 1;
    }

    return 0;
}
