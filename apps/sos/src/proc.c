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

#include <assert.h>
#include <ut_manager/ut.h>
#include <sys/panic.h>
#include "vmem_layout.h"
#include <cpio/cpio.h>
#include "elf.h"
#include <elf/elf.h>


#define verbose 5
#include <sys/debug.h>

#define TTY_PRIORITY (0)
#define TTY_EP_BADGE (101)

void
start_first_process(char *_cpio_archive, char* app_name, seL4_CPtr fault_ep)
{
    int err;

    seL4_CPtr user_ep_cap;

    /* These required for setting up the TCB */
    seL4_UserContext context;

    /* These required for loading program sections */
    char* elf_base;
    unsigned long elf_size;

    tty_test_process->p_addrspace = as_create();

    /* Create a simple 1 level CSpace */
    tty_test_process->croot = cspace_create(1);
    assert(tty_test_process->croot != NULL);

    /* Create an IPC buffer */
    tty_test_process->ipc_buffer_addr = ut_alloc(seL4_PageBits);
    conditional_panic(!tty_test_process->ipc_buffer_addr, "No memory for ipc buffer");
    err =  cspace_ut_retype_addr(tty_test_process->ipc_buffer_addr,
                                 seL4_ARM_SmallPageObject,
                                 seL4_PageBits,
                                 cur_cspace,
                                 &tty_test_process->ipc_buffer_cap);
    conditional_panic(err, "Unable to allocate page for IPC buffer");

    /* Copy the fault endpoint to the user app to enable IPC */
    user_ep_cap = cspace_mint_cap(tty_test_process->croot,
                                  cur_cspace,
                                  fault_ep,
                                  seL4_AllRights, 
                                  seL4_CapData_Badge_new(TTY_EP_BADGE));
    /* should be the first slot in the space, hack I know */
    assert(user_ep_cap == 1);

    /* Create a new TCB object */
    tty_test_process->tcb_addr = ut_alloc(seL4_TCBBits);
    conditional_panic(!tty_test_process->tcb_addr, "No memory for new TCB");
    err =  cspace_ut_retype_addr(tty_test_process->tcb_addr,
                                 seL4_TCBObject,
                                 seL4_TCBBits,
                                 cur_cspace,
                                 &tty_test_process->tcb_cap);
    conditional_panic(err, "Failed to create TCB");

    /* Configure the TCB */
    err = seL4_TCB_Configure(tty_test_process->tcb_cap, user_ep_cap, TTY_PRIORITY,
                             tty_test_process->croot->root_cnode, seL4_NilData,
                             tty_test_process->p_addrspace->vspace, seL4_NilData, PROCESS_IPC_BUFFER,
                             tty_test_process->ipc_buffer_cap);
    conditional_panic(err, "Unable to configure new TCB");

    /* Provide a logical name for the thread -- Helpful for debugging */
#ifdef SEL4_DEBUG_KERNEL
    seL4_DebugNameThread(tty_test_process->tcb_cap, app_name);
#endif

    /* parse the cpio image */
    dprintf(1, "\nStarting \"%s\"...\n", app_name);
    elf_base = cpio_get_file(_cpio_archive, app_name, &elf_size);
    conditional_panic(!elf_base, "Unable to locate cpio header");

    /* load the elf image */
    err = elf_load(tty_test_process->p_addrspace, elf_base);
    conditional_panic(err, "Failed to load elf image");

    /* Create a stack frame */
    seL4_Word initial_stack_size = 2*PAGE_SIZE_4K; /* 1 guard page */
    region *stack = as_create_region(PROCESS_STACK_TOP - initial_stack_size, initial_stack_size, seL4_CanRead | seL4_CanWrite);
    as_add_region(curproc->p_addrspace, stack);
    curproc->p_addrspace->region_stack = stack;
    
    /* Map in the IPC buffer for the thread */
    seL4_CPtr pt_cap;
    err = map_page(tty_test_process->ipc_buffer_cap, tty_test_process->p_addrspace->vspace,
                   PROCESS_IPC_BUFFER,
                   seL4_AllRights, seL4_ARM_Default_VMAttributes, &pt_cap);
    conditional_panic(err, "Unable to map IPC buffer for user app");
    /* create region for the ipc buffer */
    region *ipc_region = as_create_region(PROCESS_IPC_BUFFER, PAGE_SIZE_4K, seL4_CanRead | seL4_CanWrite);
    as_add_region(curproc->p_addrspace, ipc_region);

    /* Start the new process */
    memset(&context, 0, sizeof(context));
    context.pc = elf_getEntryPoint(elf_base);
    context.sp = PROCESS_STACK_TOP;
    seL4_TCB_WriteRegisters(tty_test_process->tcb_cap, 1, 0, 2, &context);
}
