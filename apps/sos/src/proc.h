/*
 * Process Management
 *
 * Cameron Lonsdale & Glenn McGuire
 */

#ifndef _PROC_H_
#define _PROC_H_

#include <sel4/sel4.h>
#include <cspace/cspace.h>
#include "pagetable.h"

/* maximum number of processes to support */
#define MAX_PROCS 32

/* region linked list node for a process control block */
typedef struct region {
    seL4_Word vaddr_start;              /* starting virtual address of region */
    seL4_Word vaddr_end;                /* ending virtual address of region */
    seL4_Word permissions;
    struct region *next_region;         /* pointer to next region in LL */
} vaddr_region;

/* entry for a process control block */
typedef struct {

    seL4_Word tcb_addr;
    seL4_TCB tcb_cap;

    seL4_Word vroot_addr;
    seL4_ARM_PageDirectory vroot;

    vaddr_region *region_list;              /* pointer to region LL */
    vaddr_region *region_stack;
    vaddr_region *region_heap;              /* pointer to the heap region */

    page_directory_t *page_directory;

    seL4_Word ipc_buffer_addr;  
    seL4_CPtr ipc_buffer_cap;

    cspace_t *croot;

} proc_ctl_blk;

/* global process array */
proc_ctl_blk sos_procs[MAX_PROCS];

/* hacky first proc */
#define tty_test_process (&sos_procs[0])
#define curproc (tty_test_process)

/*
 * Create a new address space region and add it to a proc region list
 * @param start the virtual addr where the region starts
 * @param end the virtual addr where the region ends
 * @param proc the process control block to add the region to
 * @returns 1 on error and 0 on success
 */
vaddr_region * proc_create_region(seL4_Word, seL4_Word, proc_ctl_blk *, seL4_Word permissions);

/*
 * Add a region to a process control block region list
 * @param proc a pointer to a process control block
 * @param region a pointer to a new region
 * @returns 1 on failure and 0 on success
 */
int proc_add_region(vaddr_region *, proc_ctl_blk *);


int proc_get_region(proc_ctl_blk *proc, seL4_Word vaddr, vaddr_region **region);

#endif /* _PROC_H_ */
