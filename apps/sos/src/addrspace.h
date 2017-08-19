/*
 * Address spaces
 * 
 * Glenn McGuire & Cameron Lonsdale
 */

#ifndef _ADDRSPACE_H_
#define _ADDRSPACE_H_

#include <sel4/sel4.h>

#include "vm.h"

/*
 * Region structure to specify regions in an address space
 */
typedef struct region_t {
    seL4_Word vaddr_start;
    seL4_Word vaddr_end;
    seL4_Word permissions;
    struct region_t *next_region;
} region;

/*
 * An address space is made of a list of regions
 * A 2 level page table, and bookkeeping of the kernel page table caps
 */
typedef struct {
    region *region_list;
    region *region_stack;
    region *region_heap;

    /* Hardware page table */
    seL4_Word vspace_addr;
    seL4_ARM_PageDirectory vspace;

    /* SOS supplied page table */
    page_directory *directory;
} addrspace;


addrspace *as_create(void);

/*
 * Create a region
 * @param start, the starting virtual address of the region
 * @param size, the size of the region
 * @param permissions, encoded permissions of the region
 * @returns 0 on success else 1
 */
region *as_create_region(seL4_Word start, seL4_Word size, seL4_Word permissions);

/*
 * Add a region to an address space
 * @param proc a pointer to a process control block
 * @param region a pointer to a new region
 * @returns 1 on failure and 0 on success
 */
int as_add_region(addrspace *as, region *new_region);

/*
 * Find the region that contains vaddr
 * @param as, the addresspace to search
 * @param vaddr, the address in the region
 * @param[out] found_region
 * @returns 0 on success, else 1
 */
int as_find_region(addrspace *as, seL4_Word vaddr, region **found_region);

#endif /* _ADDRSPACE_H_ */