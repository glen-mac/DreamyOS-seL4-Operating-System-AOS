/*
 * Process Management
 *
 * Cameron Lonsdale & Glenn McGuire
 */

#include "proc.h"

#include <sel4/sel4.h>
#include <stdlib.h>
#include "mapping.h"
#include <string.h>
#include <utils/page.h>
#include <utils/util.h>

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

#define TTY_PRIORITY (0)
#define TTY_EP_BADGE (101)

/* create new proc */
static proc * proc_create(seL4_CPtr fault_ep);

static proc *
proc_create(seL4_CPtr fault_ep) { 
    int err;

    seL4_CPtr user_ep_cap;

    /* These required for setting up the TCB */
    seL4_UserContext context;

    /* These required for loading program sections */
    char *elf_base;
    unsigned long elf_size;

    /* create new proces */
    new_proc = malloc(sizeof(proc));

    /* create new addr space */
    new_proc->p_addrspace = as_create();
    assert(new_proc->p_addrspace != NULL);

    /* create file table */
    new_proc->file_table = fdtable_create();
    assert(new_proc->file_table != NULL);

    /* Create a simple 1 level CSpace */
    new_proc->croot = cspace_create(1);
    assert(new_proc->croot != NULL);

    /* Create an IPC buffer */

    // TODO: make this a frame and pin it.
    new_proc->ipc_buffer_addr = ut_alloc(seL4_PageBits);
    conditional_panic(!new_proc->ipc_buffer_addr, "No memory for ipc buffer");
    err =  cspace_ut_retype_addr(new_proc->ipc_buffer_addr,
                                 seL4_ARM_SmallPageObject,
                                 seL4_PageBits,
                                 cur_cspace,
                                 &new_proc->ipc_buffer_cap);
    conditional_panic(err, "Unable to allocate page for IPC buffer");

    /* Copy the fault endpoint to the user app to enable IPC */
    user_ep_cap = cspace_mint_cap(new_proc->croot,
                                  cur_cspace,
                                  fault_ep,
                                  seL4_AllRights, 
                                  seL4_CapData_Badge_new(TTY_EP_BADGE));
    /* should be the first slot in the space, hack I know */
    assert(user_ep_cap == 1);

    /* Create a new TCB object */
    new_proc->tcb_addr = ut_alloc(seL4_TCBBits);
    conditional_panic(!new_proc->tcb_addr, "No memory for new TCB");
    err =  cspace_ut_retype_addr(new_proc->tcb_addr,
                                 seL4_TCBObject,
                                 seL4_TCBBits,
                                 cur_cspace,
                                 &new_proc->tcb_cap);
    conditional_panic(err, "Failed to create TCB");

    /* Configure the TCB */
    err = seL4_TCB_Configure(new_proc->tcb_cap, user_ep_cap, TTY_PRIORITY,
                             new_proc->croot->root_cnode, seL4_NilData,
                             new_proc->p_addrspace->vspace, seL4_NilData, PROCESS_IPC_BUFFER,
                             new_proc->ipc_buffer_cap);
    conditional_panic(err, "Unable to configure new TCB");


}

void
proc_start(char *_cpio_archive, char *app_name, seL4_CPtr fault_ep)
{
    proc * new_proc = proc_create(fault_ep);  

    /* Provide a logical name for the thread -- Helpful for debugging */
    #ifdef SEL4_DEBUG_KERNEL
        seL4_DebugNameThread(new_proc->tcb_cap, app_name);
    #endif

    /* ------------------------------- ELF LOADING ------------------------- */

    /* parse the cpio image */
    dprintf(1, "\nStarting \"%s\"...\n", app_name);
    elf_base = cpio_get_file(_cpio_archive, app_name, &elf_size);
    conditional_panic(!elf_base, "Unable to locate cpio header");

    /* load the elf image */
    err = elf_load(new_proc->p_addrspace, elf_base);
    conditional_panic(err, "Failed to load elf image");

    /* ------------------------------- SECTION LOADING --------------------- */

    /* Create a stack frame */
    region *stack = as_create_region(PROCESS_STACK_TOP - PAGE_SIZE_4K, PAGE_SIZE_4K, seL4_CanRead | seL4_CanWrite);
    as_add_region(curproc->p_addrspace, stack);
    curproc->p_addrspace->region_stack = stack;

    /* Map in the IPC buffer for the thread */
    seL4_CPtr pt_cap;
    err = map_page(new_proc->ipc_buffer_cap, new_process->p_addrspace->vspace,
                   PROCESS_IPC_BUFFER,
                   seL4_AllRights, seL4_ARM_Default_VMAttributes, &pt_cap);
    conditional_panic(err, "Unable to map IPC buffer for user app");

    /* create region for the ipc buffer */
    region *ipc_region = as_create_region(PROCESS_IPC_BUFFER, PAGE_SIZE_4K, seL4_CanRead | seL4_CanWrite);
    as_add_region(curproc->p_addrspace, ipc_region);

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

    /* Start the new process */
    memset(&context, 0, sizeof(context));
    context.pc = elf_getEntryPoint(elf_base);
    context.sp = PROCESS_STACK_TOP;

    seL4_TCB_WriteRegisters(new_proc->tcb_cap, 1, 0, 2, &context);
}
