#include <sel4/sel4.h>
#include <stdlib.h>
#include "proc.h"

static int proc_add_region(proc_ctl_blk *, vaddr_region *);

/*
 * Create a new address space region and add it to a proc region list
 * @param start the virtual addr where the region starts
 * @param end the virtual addr where the region ends
 * @param proc the process control block to add the region to
 * @returns 1 on error and 0 on success
 */
int
proc_create_region(seL4_Word start, seL4_Word size, proc_ctl_blk * proc) {
    int result;

    /* sanity check */
    if (proc == NULL)
        return 1;
    
    /* check for stupid things */
    //if (size < 0)
    //    return 1;
    
    /* create new region */
    vaddr_region *new_region = (vaddr_region *)malloc(sizeof(vaddr_region));
    
    /* check if alloc failed */
    if (new_region == NULL)
        return 1;

    /* alter details of new region */
    new_region->vaddr_start = start;
    new_region->vaddr_end = start + size;
    
    /* add region to proc and return result */
    result = proc_add_region(proc, new_region);
    
    /* make sure we don't leak memory on failure */
    if (result)
        free(new_region);

    return result;
}


/*
 * Add a region to a process control block region list
 * @param proc a pointer to a process control block
 * @param region a pointer to a new region
 * @returns 1 on failure and 0 on success
 */
static int
proc_add_region(proc_ctl_blk * proc, vaddr_region *region) {
    
    // TODO: make this insert in order of vaddr_start so they are in order
    
    /* early termination */
    if (proc->region_list == NULL) {
        proc->region_list = region;
        goto end_add;
    }

    vaddr_region *cur_reg = proc->region_list;
    vaddr_region *prev_reg = proc->region_list;
    while (cur_reg != NULL) {
        prev_reg = cur_reg;       
        cur_reg = cur_reg->next_region;
    }
    prev_reg->next_region = region;
    
end_add:
    region->next_region = NULL;

    return 0;
}

