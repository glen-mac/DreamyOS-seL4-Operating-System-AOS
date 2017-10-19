/*
 * Process Management
 *
 * Cameron Lonsdale & Glenn McGuire
 */

#include "proc.h"

#include <clock/clock.h>
#include <fcntl.h>
#include "mapping.h"
#include <proc/elf.h>
#include <string.h>
#include <unistd.h>
#include <utils/util.h>
#include <ut_manager/ut.h>
#include <vm/layout.h>

/* Set badge to a pid */
#define SET_PROCID_BADGE(b, pid) ((b & 0x7FFFFFF) | (pid << 21))

/* Badge constants */
#define NEW_EP_BADGE_PRIORITY (0)
#define NEW_EP_BADGE (101)

/* Maximum number of processes to support; Customisable */
#define MAX_PROCS 32

/* Global process array */
static proc *sos_procs[MAX_PROCS];

/* The last found pid */
static seL4_Word curr_pid = 0;

static proc *proc_create(void);
static int proc_next_pid(pid_t *new_pid);
static int _proc_delete(proc *victim);
static bool proc_is_waiting(proc *parent, proc *child);

pid_t
proc_bootstrap(void)
{
    pid_t pid;
    if (proc_next_pid(&pid) != 0) {
        LOG_ERROR("Failed to acquire an unused pid");
        return -1;
    }

    proc *init = proc_create();
    if (init == NULL) {
        LOG_ERROR("Failed to create a new process");
        return -1;
    }

    /* Setup specific process information for init */
    init->pid = pid;
    init->proc_name = "SOS"; /* Process for SOS itself */
    init->stime = 0;
    init->p_state = RUNNING;
    init->protected = TRUE; /* Cannot be killed */

    sos_procs[init->pid] = init;
    return pid;
}

pid_t
proc_start(char *app_name, seL4_CPtr fault_ep, pid_t parent_pid)
{
    pid_t new_pid;
    pid_t last_pid = curr_pid;

    /* Assign a PID to this proc */
    if (proc_next_pid(&new_pid) != 0) {
        LOG_ERROR("Failed to acquire an unused pid");
        curr_pid = last_pid;
        return -1;
    }

    /* Create a process struct */
    proc *new_proc = proc_create();
    if (new_proc == NULL) {
        LOG_ERROR("Failed to create a new process");
        curr_pid = last_pid;
        return -1;
    }
    new_proc->pid = new_pid;
    new_proc->ppid = parent_pid;

    /* Store new proc */
    sos_procs[new_pid] = new_proc;

    /* Create IPC buffer */
    seL4_Word paddr = ut_alloc(seL4_PageBits);
    if (paddr == (seL4_Word)NULL) {
        LOG_ERROR("Failed to allocate memory for the IPC buffer");
        /*
         * We're calling _proc_delete because we dont want parent waiting resuming logic.
         * This applies to all other delete code in this function.
         * We also dont handle if these calls fail.
         */
        _proc_delete(new_proc);
        proc_destroy(new_proc);
        return -1;
    }
    
    /* Allocate IPC buffer */
    if (cspace_ut_retype_addr(paddr, seL4_ARM_SmallPageObject, seL4_PageBits, cur_cspace, &new_proc->ipc_buffer_cap) != 0) {
        LOG_ERROR("Failed to retype memory for IPC buffer");
        _proc_delete(new_proc);
        proc_destroy(new_proc);
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
    if ((new_proc->tcb_addr = ut_alloc(seL4_TCBBits)) == (seL4_Word)NULL) {
        LOG_ERROR("Failed to allocate memory for TCB");
        _proc_delete(new_proc);
        proc_destroy(new_proc);
        return -1;
    }
    
    if (cspace_ut_retype_addr(new_proc->tcb_addr, seL4_TCBObject, seL4_TCBBits, cur_cspace, &(new_proc->tcb_cap)) != 0) {
        LOG_ERROR("Failed to retype memory for TCB");
        _proc_delete(new_proc);
        proc_destroy(new_proc);
        return -1;
    }

    /* Configure the TCB */
    if (seL4_TCB_Configure(new_proc->tcb_cap, user_ep_cap, NEW_EP_BADGE_PRIORITY,
                             new_proc->croot->root_cnode, seL4_NilData,
                             new_proc->p_addrspace->vspace, seL4_NilData, PROCESS_IPC_BUFFER,
                             new_proc->ipc_buffer_cap)) {
        LOG_ERROR("Failed to configure the TCB");
        _proc_delete(new_proc);
        proc_destroy(new_proc);
        return -1;
    }

    /* Provide a logical name for the thread -- Helpful for debugging */
#ifdef SEL4_DEBUG_KERNEL
    seL4_DebugNameThread(new_proc->tcb_cap, app_name);
#endif

    if (as_define_stack(new_proc->p_addrspace) != 0) {
        LOG_ERROR("Failed to define the stack");
        _proc_delete(new_proc);
        proc_destroy(new_proc);
        return -1;
    }

    /* Map in the IPC buffer for the thread */
    seL4_CPtr pt_cap;
    if (map_page(new_proc->ipc_buffer_cap, new_proc->p_addrspace->vspace,
        PROCESS_IPC_BUFFER, seL4_AllRights, seL4_ARM_Default_VMAttributes, &pt_cap) != 0) {
        LOG_ERROR("Failed to map IPC buffer for process");
        _proc_delete(new_proc);
        proc_destroy(new_proc);
        return -1;
    }

    /* Create region for the ipc buffer */
    if (as_define_region(new_proc->p_addrspace, PROCESS_IPC_BUFFER, PAGE_SIZE_4K, seL4_CanRead | seL4_CanWrite) != 0) {
        LOG_ERROR("Failed to define region for IPC buffer");
        _proc_delete(new_proc);
        proc_destroy(new_proc);
        return -1;
    }

    /* Open stdout and stderr */
    file *open_file;

    /* STDOUT */
    if (file_open("console", O_WRONLY, &open_file) != 0) {
        LOG_ERROR("Failed to open STDOUT");
        _proc_delete(new_proc);
        proc_destroy(new_proc);
        return -1;
    }
    fdtable_insert(new_proc->file_table, STDOUT_FILENO, open_file);

    /* STDERR */
    if (file_open("console", O_WRONLY, &open_file) != 0) {
        LOG_ERROR("Failed to open STDERR");
        _proc_delete(new_proc);
        proc_destroy(new_proc);
        return -1;
    }
    fdtable_insert(new_proc->file_table, STDERR_FILENO, open_file);  

    /* Set the start time */
    new_proc->stime = time_stamp();
    
    /* Copy the name and null terminate */
    new_proc->proc_name = strdup(app_name);

    proc *parent = get_proc(parent_pid);
    assert(parent != NULL);
    if (list_prepend(parent->children, (void *)new_pid) != 0) {
        LOG_ERROR("Failed to register process as a child");
        _proc_delete(new_proc);
        proc_destroy(new_proc);
        return -1;
    }

    /* Attempt to load the ELF */
    uint64_t elf_pc = 0;
    uint32_t last_section;
    if (elf_load(new_proc, app_name, &elf_pc, &last_section) != 0) {
        LOG_ERROR("Failed to load elf file");
        _proc_delete(new_proc);
        proc_destroy(new_proc);
        return -1;
    }

    /* Define the heap, a page after the last section in the elf */
    seL4_Word heap_loc = PAGE_ALIGN_4K(last_section + PAGE_SIZE_4K);
    if (as_define_heap(new_proc->p_addrspace, heap_loc) != 0) {
        LOG_ERROR("Failed to define the heap");
        _proc_delete(new_proc);
        proc_destroy(new_proc);
        return -1;
    }

    /* Start the new process */
    seL4_UserContext context;
    memset(&context, 0, sizeof(context));
    context.pc = elf_pc;
    context.sp = PROCESS_STACK_TOP;
    seL4_TCB_WriteRegisters(new_proc->tcb_cap, 1, 0, 2, &context);

    new_proc->p_state = RUNNING;
    return new_pid;
}

int
proc_delete(proc *victim)
{
    LOG_INFO("%d being killed", victim->pid);

    if (_proc_delete(victim) != 0) {
        LOG_ERROR("Failed to delete process completely");
        return 1;
    }

    proc *parent = get_proc(victim->ppid);
    if (parent == NULL) {
        LOG_ERROR("Failed to find parent process");
        return 1;
    }

    /* Resume a parent waiting for this process to exit */
    if (proc_is_waiting(parent, victim))
        resume(parent->waiting_coro, (void *)victim->pid);

    return 0;
}

void
proc_destroy(proc *victim)
{
    proc *parent = get_proc(victim->ppid);
    assert(parent != NULL);

    /* Remove proc as a child of its parent */
    LOG_INFO("Removing %d as child of %d", victim->pid, parent->pid);
    list_remove(parent->children, (void *)victim->pid, list_cmp_equality);

    /* Destroy the (now empty) list of children */
    assert(list_is_empty(victim->children));
    list_destroy(victim->children);
    victim->children = NULL;

    /* Release the name of the process */
    free(victim->proc_name);
    victim->proc_name = NULL;

    sos_procs[victim->pid] = NULL;
    free(victim);
}

bool
proc_is_child(proc *curproc, pid_t pid)
{
    return list_exists(curproc->children, (void *)pid, list_cmp_equality);
}

proc *
get_proc(pid_t pid)
{
    if (!ISINRANGE(0, pid, MAX_PROCS)) {
        LOG_ERROR("pid %d out of bounds", pid);
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
    if (new_parent_proc == NULL) {
        LOG_ERROR("Failed to find parent process");
        return 1;
    }

    for (struct list_node *curr = cur_parent->children->head; curr != NULL; curr = curr->next) {
        proc *child = get_proc((pid_t)curr->data);
        if (child == NULL) {
            LOG_ERROR("Failed to find child process");
            return 1;
        }
        child->ppid = new_parent_proc->pid;
        list_prepend(new_parent_proc->children, curr->data);
    }

    /* Safetly remove all children from the current processes list */
    list_remove_all(cur_parent->children);

    return 0;
}

/*
 * Check if parent is waiting on child
 * @param parent, parent process
 * @param child, child process
 * @returns true or false
 */
static bool
proc_is_waiting(proc *parent, proc *child)
{
    /*
     * Parent either has to be waiting for any child (-1) or child pid specifically
     * and they have a coroutine saved to resume
     */
    return ((parent->waiting_on == -1 || parent->waiting_on == child->pid) && parent->waiting_coro);
}

/*
 * Create a new process struct
 * Initialise an addresspace, filetable, cspace, and list of children
 * The returned process has the CREATED state
 * @returns pointer to newly created process
 */
static proc *
proc_create(void)
{
    proc *new_proc = malloc(sizeof(proc));
    if (new_proc == NULL) {
        LOG_ERROR("Failed to acquire memory for new process");
        return NULL;
    }

    if ((new_proc->p_addrspace = as_create()) == NULL) {
        LOG_ERROR("Failed to create an addrspace");
        return NULL;
    }

    if ((new_proc->file_table = fdtable_create()) == NULL) {
        LOG_ERROR("Failed to create an fdtable");
        return NULL;
    }

    if ((new_proc->children = malloc(sizeof(list_t))) == NULL) {
        LOG_ERROR("Failed to create a list of children");
        return NULL;
    }
    list_init(new_proc->children);

    /* Create a simple 1 level CSpace */
    if ((new_proc->croot = cspace_create(1)) == NULL) {
        LOG_ERROR("Failed to create a cspace");
        return NULL;
    }

    new_proc->p_state = CREATED; /* Not yet running */
    new_proc->protected = FALSE; /* Process can be killed */
    new_proc->blocked_ref = 0; /* Not currently blocked */

    /* Set other elements to void values */
    new_proc->tcb_addr = (seL4_Word)NULL;
    new_proc->tcb_cap = (seL4_TCB)NULL;
    new_proc->ipc_buffer_cap = (seL4_CPtr)NULL;
    new_proc->waiting_on = -1;
    new_proc->waiting_coro = NULL;
    new_proc->ppid = -1;
    new_proc->pid = -1;
    new_proc->proc_name = NULL;
    new_proc->stime = -1;
    new_proc->kill_flag = FALSE;

    return new_proc;
}

/*
 * Find the next available pid
 * Linear search through the process array
 * @param[out] new_pid, the available pid
 * @returns 0 on success, else 1
 */
static int
proc_next_pid(pid_t *new_pid)
{
    seL4_Word start_pid = curr_pid;
    
    /* Early return if the current pid is free */
    if (sos_procs[curr_pid] == NULL)
        goto return_pid;
 
    /* Loop until we find a free pid */
    do {
        curr_pid = (curr_pid + 1) % MAX_PROCS;
        if (sos_procs[curr_pid] == NULL)
            goto return_pid;
    } while (curr_pid != start_pid);

    /* Out of pids */
    return 1;

    return_pid:
        *new_pid = curr_pid;
        /* Advanced curr_pid for next use */
        curr_pid = (curr_pid + 1) % MAX_PROCS;
        return 0;
}

/*
 * Delete all resources associated with a process
 * @param victim, the process to delete
 * @returns 0 on success else 1
 */
static int
_proc_delete(proc *victim)
{
    if (victim->protected) {
        LOG_ERROR("Cannot kill a protected process");
        return 1;
    }

    if (victim->p_state == ZOMBIE) {
        LOG_ERROR("Cannot delete a zombie process");
        return 1;
    }
    victim->p_state = ZOMBIE;

    /* Stop the thread if currently running */
    if (victim->tcb_cap && seL4_TCB_Suspend(victim->tcb_cap) != 0) {
        LOG_ERROR("Failed to suspend thread");
        return 1;
    }

    /* TCB Destroy if existing */
    if (victim->tcb_cap && cspace_delete_cap(cur_cspace, victim->tcb_cap) != CSPACE_NOERROR) {
        LOG_ERROR("Failed to delete cap for TCB");
        return 1;
    }

    victim->tcb_cap = (seL4_TCB)NULL;
    if (victim->tcb_addr)
        ut_free(victim->tcb_addr, seL4_TCBBits);
    victim->tcb_addr = (seL4_Word)NULL;

    /* IPC destroy if existing */
    if (victim->ipc_buffer_cap && seL4_ARM_Page_Unmap(victim->ipc_buffer_cap) != 0) {
        LOG_ERROR("Failed to unmap IPC buffer");
        return 1;
    }

    if (victim->ipc_buffer_cap && cspace_delete_cap(cur_cspace, victim->ipc_buffer_cap) != 0) {
        LOG_ERROR("Failed to delete IPC buffer cap");
        return 1;
    }
    victim->ipc_buffer_cap = (seL4_CPtr)NULL;

    /* Destroy the cspace if existing */
    if (victim->croot && cspace_destroy(victim->croot) != CSPACE_NOERROR) {
        LOG_ERROR("Failed to destroy cspace");
        return 1;
    }
    victim->croot = NULL;

    /* Destroy the addrspace if existing */
    if (victim->p_addrspace && as_destroy(victim->p_addrspace) != 0) {
        LOG_ERROR("Failed to destroy addrspace");
        return 1;
    }
    victim->p_addrspace = NULL;

    /* Destroy the fdtable if existing */
    if (victim->file_table && fdtable_destroy(victim->file_table) != 0) {
        LOG_ERROR("Failed to destroy fdtable");
        return 1;
    }
    victim->file_table = NULL;

    /* Reparent all children to the init process, if children */
    if (victim->children && proc_reparent_children(victim, 0) != 0) {
        LOG_ERROR("Failed to reparent children");
        return 1;
    }

    return 0;
}
