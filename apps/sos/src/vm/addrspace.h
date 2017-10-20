/*
 * Address spaces
 * 
 * Glenn McGuire & Cameron Lonsdale
 */

#ifndef _ADDRSPACE_H_
#define _ADDRSPACE_H_

#include <sel4/sel4.h>
#include <stdbool.h>
#include <stdint.h>

/* Maximum size limit of the stack region (16mb) */
#define RLIMIT_STACK_SZ 16780000

/* Forward declaration of a page directory */
typedef struct page_dir page_directory;

/*
 * Region structure to specify regions in an address space
 * Each region has a start and end address, access permissions
 * and a pointer to the next region in the linked list
 */
typedef struct region_t {
    seL4_Word start;
    seL4_Word end;
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

/*
 * Create an address space
 * @returns a pointer to an addrspace on success, else NULL
 */
addrspace *as_create(void);

/*
 * Destroy an address space
 * @param as, the addrspace to destroy
 * @returns 0 on success else 1
 */
int as_destroy(addrspace *as);

/*
 * Create a region
 * @param start, the starting virtual address of the region
 * @param size, the size of the region, in bytes
 * @param permissions, encoded permissions of the region
 * @returns region pointer, or NULL on error
 */
region *as_create_region(seL4_Word start, seL4_Word size, seL4_Word permissions);

/*
 * Destroy a region
 * @param reg, the region to destroy
 * @returns 0 on success, else 1
 */
int as_destroy_region(region *reg);

/*
 * Add a region to an address space
 * @param as, pointer to the addrspace to add the region to
 * @param new_region, a pointer to a new region
 * @returns 0 on success, else 1
  */
int as_add_region(addrspace *as, region *new_region);

/*
 * Create and add a region to an address space
 * @param as, the address space to add the region into
 * @param start, the starting virtual address of the region
 * @param size, the size of the region
 * @param permissions, encoded permissions of the region
 * @returns 0 on success, else 1
 */
int as_define_region(addrspace *as, seL4_Word start, seL4_Word size, seL4_Word permissions);

/*
 * Find the region that contains vaddr
 * @param as, the addresspace to search
 * @param vaddr, the address in the region
 * @param[out] found_region, the found region
 * @returns 0 on success, else 1
 */
int as_find_region(addrspace *as, seL4_Word vaddr, region **found_region);

/*
 * Check if the given start and end collide with existing regions in an addrspace
 * @param as, the addresspace to search
 * @param exempt, the region you're trying to expand, so dont check this
 * @param start, the starting address of the region
 * @param end, the ending address of the region
 * @returns 0 on success, else 1
 */
int as_region_collision_check(addrspace *as, region *exempt, seL4_Word start, seL4_Word end);

/*
 * Check if the region has correct permissions for a certain type of access
 * @param reg, the region to check
 * @param access_type, the type of access
 * @return TRUE if correct permissions, else FALSE
 */ 
bool as_region_permission_check(region *reg, seL4_Word access_type);

/*
 * Define a stack region for an address space
 * @param as, the address space to define the stack in
 * @returns 0 on success, else 1
 */
int as_define_stack(addrspace *as);

/*
 * Define a heap region for an address space
 * @param as, the address space to define the heap in
 * @returns 0 on success, else 1
 */
int as_define_heap(addrspace *as, uint32_t heap_loc);

#endif /* _ADDRSPACE_H_ */
