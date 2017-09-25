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
static proc *proc_create(seL4_CPtr fault_ep, pid_t new_pid);

/* gets/sets the next free pid */
static int proc_next_pid(pid_t *new_pid);

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
    proc *new_proc = proc_create(fault_ep, new_pid);
    if (new_proc == NULL) {
        LOG_ERROR("proc_create failed");
        curr_pid = last_pid;
        return -1;
    }

    new_proc->ppid = parent_pid;
    new_proc->pid = new_pid;
    new_proc->waiting_on = -1;
    new_proc->waiting_coro = NULL;

    /* Store new proc */
    sos_procs[new_pid] = new_proc;

    /* Provide a logical name for the thread -- Helpful for debugging */
#ifdef SEL4_DEBUG_KERNEL
    seL4_DebugNameThread(new_proc->tcb_cap, app_name);
#endif

    if ((elf_base = cpio_get_file(_cpio_archive, app_name, &elf_size)) == NULL) {
        LOG_ERROR("Unable to locate cpio header");
        return -1;
    }

    if (elf_load(new_proc, elf_base) != 0) {
        LOG_ERROR("Error loading elf");
        return -1;
    }

    if (as_define_stack(new_proc->p_addrspace) != 0) {
        LOG_ERROR("Error defining stack");
        return -1;
    }

    /* Map in the IPC buffer for the thread */
    seL4_CPtr pt_cap;
    if (map_page(new_proc->ipc_buffer_cap, new_proc->p_addrspace->vspace,
        PROCESS_IPC_BUFFER, seL4_AllRights, seL4_ARM_Default_VMAttributes, &pt_cap) != 0) {
        LOG_ERROR("Unable to map IPC buffer for user app");
        return -1;
    }

    /* create region for the ipc buffer */
    region *ipc_region = as_create_region(PROCESS_IPC_BUFFER, PAGE_SIZE_4K, seL4_CanRead | seL4_CanWrite);
    as_add_region(new_proc->p_addrspace, ipc_region);

    /* -------------------------------- FD OPENINING ----------------------- */

    /* Open stdin, stdout and stderr */
    file *open_file;

    /* STDIN */
    err = file_open("console", O_RDONLY, &open_file);
    conditional_panic(err, "Unable to open stdin");
    fdtable_insert(new_proc->file_table, STDIN_FILENO, open_file);

    /* STDOUT */
    err = file_open("console", O_WRONLY, &open_file);
    conditional_panic(err, "Unable to open stdout");
    fdtable_insert(new_proc->file_table, STDOUT_FILENO, open_file);

    /* STDERR */
    err = file_open("console", O_WRONLY, &open_file);
    conditional_panic(err, "Unable to open stderr");
    fdtable_insert(new_proc->file_table, STDERR_FILENO, open_file);    

    /* ------------------------------ START PROC --------------------------- */

    /* set the start time */
    new_proc->stime = time_stamp();
    new_proc->p_state = RUNNING;

    /* Start the new process */
    seL4_UserContext context;
    memset(&context, 0, sizeof(context));
    context.pc = elf_getEntryPoint(elf_base);
    context.sp = PROCESS_STACK_TOP;

    seL4_TCB_WriteRegisters(new_proc->tcb_cap, 1, 0, 2, &context);
    
    /* copy the name and null terminate */
    memcpy(new_proc->proc_name, app_name, N_NAME);
    new_proc->proc_name[31] = NULL;

    return new_pid;
}

void
proc_destroy(proc *current)
{
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
proc_create(seL4_CPtr fault_ep, pid_t new_pid)
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
    
    /* Create IPC buffer */
    seL4_Word paddr = ut_alloc(seL4_PageBits);
    if (paddr == NULL) {
        LOG_ERROR("No memory for IPC buffer");
        return NULL;
    }
    
    if (cspace_ut_retype_addr(paddr, seL4_ARM_SmallPageObject, seL4_PageBits,
                              cur_cspace, &new_proc->ipc_buffer_cap) != 0) {
        LOG_ERROR("Unable to allocate page for IPC buffer");
    }

    /* Copy the fault endpoint to the user app to enable IPC */
    seL4_CPtr user_ep_cap = cspace_mint_cap(
        new_proc->croot, cur_cspace, fault_ep, seL4_AllRights,
        seL4_CapData_Badge_new(SET_PROCID_BADGE(NEW_EP_BADGE, new_pid))
    );

    /* Should be the first slot in the space, hack I know */
    assert(user_ep_cap == 1);

    /* Create a new TCB object */
    paddr = ut_alloc(seL4_TCBBits);
    if (paddr == NULL) {
        LOG_ERROR("No memory for TCB");
        return NULL;
    }
    
    if (cspace_ut_retype_addr(paddr, seL4_TCBObject, seL4_TCBBits, cur_cspace, &new_proc->tcb_cap) != 0) {
        LOG_ERROR("Failed to create TCB");
        return NULL;
    }

    /* Configure the TCB */
    if (seL4_TCB_Configure(new_proc->tcb_cap, user_ep_cap, NEW_EP_BADGE_PRIORITY,
                             new_proc->croot->root_cnode, seL4_NilData,
                             new_proc->p_addrspace->vspace, seL4_NilData, PROCESS_IPC_BUFFER,
                             new_proc->ipc_buffer_cap)) {
        LOG_ERROR("Unable to configure new TCB");
        return NULL;
    }

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
