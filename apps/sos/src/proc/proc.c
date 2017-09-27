/*
 * Process Management
 *
 * Cameron Lonsdale & Glenn McGuire
 */

#include "proc.h"

#include "event.h"

#include <sel4/sel4.h>
#include <stdlib.h>
#include "mapping.h"
#include <string.h>
#include <utils/page.h>
#include <utils/util.h>
#include <clock/clock.h>

#include <assert.h>
#include <ut_manager/ut.h>
#include <sys/panic.h>
#include <vm/layout.h>
#include <cpio/cpio.h>
#include "elf.h"
#include <elf/elf.h>

#include <unistd.h>
#include <fcntl.h>

#define verbose 5
#include <sys/debug.h>

#define NEW_EP_BADGE_PRIORITY (0)
#define NEW_EP_BADGE (101)

/* Maximum number of processes to support */
#define MAX_PROCS 32

/* Global process array */
proc *sos_procs[MAX_PROCS];

/* the last found pid */
static seL4_Word curr_pid = 0;

/* create new proc */
static proc *proc_create(void);

/* gets/sets the next free pid */
static int proc_next_pid(pid_t *new_pid);

pid_t proc_bootstrap(void)
{
    pid_t init_pid;
    if (proc_next_pid(&init_pid) != 0) {
        LOG_ERROR("proc_next_pid failed");
        return -1;
    }

    /* Create a process struct */
    proc *init = proc_create();
    if (init == NULL) {
        LOG_ERROR("proc_create failed");
        return -1;
    }

    init->pid = init_pid;
    init->ppid = -1;
    init->proc_name = "SOS";
    init->stime = 0;
    init->p_state = RUNNING;
    init->protected = TRUE;
    init->waiting_coro = NULL;
    sos_procs[init->pid] = init;
    return init->pid;
}

pid_t
proc_start(char *_cpio_archive, char *app_name, seL4_CPtr fault_ep, pid_t parent_pid)
{
    int err;
    pid_t new_pid;
    pid_t last_pid = curr_pid;
    
    /* These required for loading program sections */
    char *elf_base;
    unsigned long elf_size;

    /* Assign a PID to this proc */
    if (proc_next_pid(&new_pid) != 0) {
        LOG_ERROR("proc_next_pid failed");
        curr_pid = last_pid;
        return -1;
    }

    /* Create a process struct */
    proc *new_proc = proc_create();
    if (new_proc == NULL) {
        LOG_ERROR("proc_create failed");
        curr_pid = last_pid;
        return -1;
    }
    new_proc->pid = new_pid;

    /* Create IPC buffer */
    seL4_Word paddr = ut_alloc(seL4_PageBits);
    if (paddr == NULL) {
        LOG_ERROR("No memory for IPC buffer");
        return -1;
    }
    
    /* Allocate IPC buffer */
    if (cspace_ut_retype_addr(paddr, seL4_ARM_SmallPageObject, seL4_PageBits,
                              cur_cspace, &new_proc->ipc_buffer_cap) != 0) {
        LOG_ERROR("Unable to allocate page for IPC buffer");
        return -1;
    }

    /* Copy the fault endpoint to the user app to enable IPC */
    seL4_CPtr user_ep_cap = cspace_mint_cap(
        new_proc->croot, cur_cspace, fault_ep, seL4_AllRights,
        seL4_CapData_Badge_new(SET_PROCID_BADGE(NEW_EP_BADGE, new_pid))
    );

    /* Should be the first slot in the space, hack I know */
    assert(user_ep_cap == 1);

    /* Create a new TCB object */
    if ((new_proc->tcb_addr = ut_alloc(seL4_TCBBits)) == NULL) {
        LOG_ERROR("No memory for TCB");
        return -1;
    }
    
    if (cspace_ut_retype_addr(new_proc->tcb_addr, seL4_TCBObject, seL4_TCBBits, cur_cspace, &(new_proc->tcb_cap)) != 0) {
        LOG_ERROR("Failed to create TCB");
        return -1;
    }

    /* Configure the TCB */
    if (seL4_TCB_Configure(new_proc->tcb_cap, user_ep_cap, NEW_EP_BADGE_PRIORITY,
                             new_proc->croot->root_cnode, seL4_NilData,
                             new_proc->p_addrspace->vspace, seL4_NilData, PROCESS_IPC_BUFFER,
                             new_proc->ipc_buffer_cap)) {
        LOG_ERROR("Unable to configure new TCB");
        return -1;
    }

    new_proc->ppid = parent_pid;
    new_proc->pid = new_pid;
    new_proc->waiting_on = -1;
    new_proc->waiting_coro = NULL;
    new_proc->kill_flag = FALSE;

    /* Provide a logical name for the thread -- Helpful for debugging */
#ifdef SEL4_DEBUG_KERNEL
    seL4_DebugNameThread(new_proc->tcb_cap, app_name);
#endif

    if ((elf_base = cpio_get_file(_cpio_archive, app_name, &elf_size)) == NULL) {
        LOG_ERROR("Unable to locate cpio header");
        proc_delete(new_proc);
        proc_destroy(new_proc);
        return -1;
    }

    if (elf_load(new_proc, elf_base) != 0) {
        LOG_ERROR("Error loading elf");
        proc_delete(new_proc);
        proc_destroy(new_proc);
        return -1;
    }

    if (as_define_stack(new_proc->p_addrspace) != 0) {
        LOG_ERROR("Error defining stack");
        proc_delete(new_proc);
        proc_destroy(new_proc);
        return -1;
    }

    /* Map in the IPC buffer for the thread */
    seL4_CPtr pt_cap;
    if (map_page(new_proc->ipc_buffer_cap, new_proc->p_addrspace->vspace,
        PROCESS_IPC_BUFFER, seL4_AllRights, seL4_ARM_Default_VMAttributes, &pt_cap) != 0) {
        LOG_ERROR("Unable to map IPC buffer for user app");
        proc_delete(new_proc);
        proc_destroy(new_proc);
        return -1;
    }

    /* create region for the ipc buffer */
    region *ipc_region = as_create_region(PROCESS_IPC_BUFFER, PAGE_SIZE_4K, seL4_CanRead | seL4_CanWrite);
    as_add_region(new_proc->p_addrspace, ipc_region);

    /* -------------------------------- FD OPENINING ----------------------- */

    /* Open stdin, stdout and stderr */
    file *open_file;

    /* STDIN */
    if (file_open("console", O_RDONLY, &open_file) != 0) {
        LOG_ERROR("Unable to open STDIN");
        proc_delete(new_proc);
        proc_destroy(new_proc);
        return -1;
    }
    fdtable_insert(new_proc->file_table, STDIN_FILENO, open_file);

    /* STDOUT */
    if (file_open("console", O_WRONLY, &open_file) != 0) {
        LOG_ERROR("Unable to open STDOUT");
        proc_delete(new_proc);
        proc_destroy(new_proc);
        return -1;
    }
    fdtable_insert(new_proc->file_table, STDOUT_FILENO, open_file);

    /* STDERR */
    if (file_open("console", O_WRONLY, &open_file) != 0) {
        LOG_ERROR("Unable to open STDERR");
        proc_delete(new_proc);
        proc_destroy(new_proc);
        return -1;
    }
    fdtable_insert(new_proc->file_table, STDERR_FILENO, open_file);    

    /* ------------------------------ START PROC --------------------------- */

    /* Set the start time */
    new_proc->stime = time_stamp();
    new_proc->p_state = RUNNING;

    /* Start the new process */
    seL4_UserContext context;
    memset(&context, 0, sizeof(context));
    context.pc = elf_getEntryPoint(elf_base);
    context.sp = PROCESS_STACK_TOP;

    seL4_TCB_WriteRegisters(new_proc->tcb_cap, 1, 0, 2, &context);
    
    /* Copy the name and null terminate */
    new_proc->proc_name = strdup(app_name);

    proc *parent = get_proc(parent_pid);
    assert(parent != NULL);
    if (list_prepend(parent->children, new_pid) != 0) {
        LOG_ERROR("Error creating child");
        proc_delete(new_proc);
        proc_destroy(new_proc);
        return -1;
    }

    /* Store new proc */
    sos_procs[new_pid] = new_proc;

    return new_pid;
}

int
proc_delete(proc *victim)
{
    int ret_val = 1;
    if (victim->protected) {
        LOG_ERROR("Cannot kill a protected process");
        goto error;
    }

    if (victim->p_state == ZOMBIE) {
        LOG_ERROR("Cannot delete zombie processes");
        goto error;
    }
    victim->p_state = ZOMBIE;

    /* Stop the thread */
    if (seL4_TCB_Suspend(victim->tcb_cap) != 0) {
        LOG_ERROR("Failed to suspend thread");
        goto error;
    }

    /* TCB Destroy */
    if (cspace_delete_cap(cur_cspace, victim->tcb_cap) != CSPACE_NOERROR) {
        LOG_ERROR("Failed to delete cap for TCB");
        goto error;
    }
    victim->tcb_cap = NULL;
    ut_free(victim->tcb_addr, seL4_TCBBits);
    victim->tcb_addr = NULL;

    /* IPC destroy */
    if (seL4_ARM_Page_Unmap(victim->ipc_buffer_cap) != 0) {
        LOG_ERROR("Failed to unmap IPC buffer");
        goto error;
    }

    if (cspace_delete_cap(cur_cspace, victim->ipc_buffer_cap) != 0) {
        LOG_ERROR("Failed to delete IPC buffer cap");
        goto error;
    }
    victim->ipc_buffer_cap = NULL;

    /* Destroy the Cspace */
    if (cspace_destroy(victim->croot) != CSPACE_NOERROR) {
        LOG_ERROR("Failed to destroy cspace");
        goto error;
    }
    victim->croot = NULL;

    /* Destroy the address space */
    if (as_destroy(victim->p_addrspace) != 0) {
        LOG_ERROR("Failed to destroy addrspace");
        goto error;
    }
    victim->p_addrspace = NULL;

    /* Destroy the file table */
    if (fdtable_destroy(victim->file_table) != 0) {
        LOG_ERROR("Failed to destroy file table");
        goto error;
    }
    victim->file_table = NULL;

    LOG_INFO("BEFORE REPARENT");

    /* Reparent all children to the init process */
    if (proc_reparent_children(victim, 0) != 0) {
        LOG_ERROR("Failed to reparent children");
        goto error;
    }

    LOG_INFO("AFTER REPARENT");

    proc *parent = get_proc(victim->ppid);
    if (!parent) {
        LOG_ERROR("Unable to find parent");
        goto error;
    }

    if (proc_is_waiting(parent, victim)) {
        LOG_INFO("resuming coroutine");
        resume(parent->waiting_coro, victim->pid);
    }

    ret_val = 0;
    error:
        return ret_val;
}

void
proc_destroy(proc *current)
{
    /* Remove proc as child from parent */
    proc *parent = get_proc(current->ppid);
    LOG_INFO("removing %d from %d", current->pid, parent->pid);
    list_remove(parent->children, current->pid, list_cmp_equality);

    sos_procs[current->pid] = NULL;
    free(current);
}

bool
proc_is_child(proc *curproc, pid_t pid)
{
    return list_exists(curproc->children, pid, list_cmp_equality);
}

bool
proc_is_waiting(proc *parent, proc *child)
{
    return ((parent->waiting_on == -1 || parent->waiting_on == child->pid) && parent->waiting_coro);
}

static proc *
proc_create(void)
{
    proc *new_proc = malloc(sizeof(proc));
    if (!new_proc)
        return NULL;

    if ((new_proc->p_addrspace = as_create()) == NULL) {
        LOG_ERROR("as_create failed");
        return NULL;
    }

    if ((new_proc->file_table = fdtable_create()) == NULL) {
        LOG_ERROR("fdtable_create failed");
        return NULL;
    }

    if ((new_proc->children = malloc(sizeof(list_t))) == NULL) {
        LOG_ERROR("failed to create list of children");
        return NULL;
    }
    assert(list_init(new_proc->children) == 0);

    /* Create a simple 1 level CSpace */
    if ((new_proc->croot = cspace_create(1)) == NULL) {
        LOG_ERROR("cspace_create failed");
        return NULL;
    }

    new_proc->protected = FALSE;
    new_proc->blocked_ref = 0;

    return new_proc;
}

static int
proc_next_pid(pid_t *new_pid) {
    seL4_Word start_pid = curr_pid;
    
    /* early return on next pid was free */
    if (sos_procs[curr_pid] == NULL) {
        goto return_pid;
    }
 
    /* loop around until we find a free pid */
    do {
        curr_pid = (curr_pid + 1) % MAX_PROCS;
        if (sos_procs[curr_pid] == NULL)
            goto return_pid;
    } while (curr_pid != start_pid);

    /* if we got here, we reached start of loop and are out of pids */
    return 1;

    /* else return the free pid */
    return_pid:
        *new_pid = curr_pid;
        curr_pid = (curr_pid + 1) % MAX_PROCS;
        return 0;
}

proc *
get_proc(pid_t pid)
{
    if (!ISINRANGE(0, pid, MAX_PROCS)) {
        LOG_ERROR("pid: %d out of bounds", pid);
        return NULL;
    }

    return sos_procs[pid];
}

void
proc_mark(proc *curproc, proc_states state)
{
    curproc->p_state = state;
}

int
proc_reparent_children(proc *cur_parent, pid_t new_parent)
{
    proc *new_parent_proc = get_proc(new_parent);
    if (new_parent_proc == NULL)
        return 1;

    LOG_INFO("cur_parent is %p", cur_parent);
    LOG_INFO("cur_parent children is %p", cur_parent->children);
    for (struct list_node *curr = cur_parent->children->head; curr != NULL; curr = curr->next) {
        LOG_INFO("curr is %p", curr);
        LOG_INFO("curr->data is %d", curr->data);
        proc *child = get_proc(curr->data);
        LOG_INFO("child is %p", child);
        child->ppid = new_parent_proc->pid;
        list_prepend(new_parent_proc->children, curr->data);
    }

    return 0;
}
