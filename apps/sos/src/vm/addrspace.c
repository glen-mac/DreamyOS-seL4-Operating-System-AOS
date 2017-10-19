/*
 * Address spaces
 * 
 * Glenn McGuire & Cameron Lonsdale
 */

#include "addrspace.h"

#include "vm.h"
#include "layout.h"
#include <ut_manager/ut.h>
#include <utils/util.h>

#define WITHIN_REGION(as, addr) (addr >= as->start && addr < as->end) 

addrspace *
as_create(void)
{
    addrspace *as;
    if ((as = malloc(sizeof(addrspace))) == NULL) {
        LOG_ERROR("Failed to allocate memory for addrspace");
        return NULL;
    }

    as->region_list = NULL;
    as->region_stack = NULL;
    as->region_heap = NULL;

    /* Create a VSpace */
    if ((as->vspace_addr = ut_alloc(seL4_PageDirBits)) == (seL4_Word)NULL) {
        LOG_ERROR("Failed to create a VSpace");
        free(as);
        return NULL;
    }

    if (cspace_ut_retype_addr(as->vspace_addr, seL4_ARM_PageDirectoryObject, seL4_PageDirBits, cur_cspace, &(as->vspace))) {
        LOG_ERROR("Failed to retype VSpace");
        ut_free(as->vspace_addr, seL4_PageDirBits);
        free(as);
        return NULL;
    }

    /* Create the top level of the page table */
    if ((as->directory = page_directory_create()) == NULL) {
        LOG_ERROR("Failed to create a page directory");
        cspace_delete_cap(cur_cspace, as->vspace);
        ut_free(as->vspace_addr, seL4_PageBits);
        free(as);
        return NULL;
    }

    return as;
}

int 
as_destroy(addrspace *as)
{
    if (page_directory_destroy(as->directory) != 0) {
        LOG_ERROR("Failed to destroy page directory");
        return 1;
    }
    as->directory = NULL;

    /* Destroy hardware page table */
    if (cspace_delete_cap(cur_cspace, as->vspace) == CSPACE_ERROR) {
        LOG_ERROR("Failed to delete vspace cap");
        return 1;
    }
    as->vspace = (seL4_Word)NULL;

    ut_free(as->vspace_addr, seL4_PageDirBits);
    as->vspace_addr = (seL4_Word)NULL;

    for (region *curr = as->region_list; curr != NULL; curr = curr->next_region) {
        if (as_destroy_region(curr) != 0) {
            LOG_ERROR("Failed to destroy region");
            return 1;
        }
    }
    as->region_list = NULL;

    free(as);
    return 0;
}

region *
as_create_region(seL4_Word start, seL4_Word size, seL4_Word permissions)
{
    assert(size >= 0);
    region *new_region;
    if ((new_region = (region *)malloc(sizeof(region))) == NULL) {
        LOG_ERROR("Failed to allocate space for a region");
        return NULL;
    }

    new_region->start = start;
    new_region->end = start + size;
    new_region->permissions = permissions;

    return new_region;
}

int
as_destroy_region(region *reg)
{
    if (reg == NULL) {
        LOG_ERROR("Failed to destroy region");
        return 1;
    }

    free(reg);
    return 0;
}

int
as_add_region(addrspace *as, region *new_region)
{
    LOG_INFO("Adding region %p -> %p", (void *)new_region->start, (void *)new_region->end);

    /* If there are no previous regions */
    if (as->region_list == NULL) {
        as->region_list = new_region;
        goto end_add;
    }

    /* Add region to the end of the list */
    region *cur_reg = as->region_list;
    region *prev_reg = as->region_list;
    while (cur_reg != NULL) {
        prev_reg = cur_reg;       
        cur_reg = cur_reg->next_region;
    }
    prev_reg->next_region = new_region;
    
    end_add:
        new_region->next_region = NULL;

    return 0;
}

int
as_define_region(addrspace *as, seL4_Word start, seL4_Word size, seL4_Word permissions)
{
    return as_add_region(as, as_create_region(start, size, permissions));
}

int
as_find_region(addrspace *as, seL4_Word vaddr, region **found_region)
{
    /* Search through the linked list until found */
    region *curr = as->region_list;
    while (curr != NULL) {
        if (vaddr >= curr->start && vaddr < curr->end) {
            *found_region = curr;
            return 0;
        }
        curr = curr->next_region;
    }

    return 1;
}

int
as_region_collision_check(addrspace *as, seL4_Word start, seL4_Word end)
{
   if (start >= as->region_heap->end && start < as->region_stack->start)
      return 0;

   return 1;

   /*
   TODO; Probably need to take in the region these values correspond to, cause 
   we want to ignore that region we are checking

   region *curr = as->region_list;
   while (curr != NULL) {
        if (WITHIN_REGION(curr, start) || WITHIN_REGION(curr, end)) {   
            return 0;
        }
        curr = curr->next_region;
   }
   return 1;
   */
}

bool
as_region_permission_check(region *reg, seL4_Word access_type)
{
    if (access_type == ACCESS_READ && (reg->permissions & seL4_CanRead))
        return TRUE; /* Correct Permission */

    if (access_type == ACCESS_WRITE && (reg->permissions & seL4_CanWrite))
        return TRUE; /* Correct Permission */

    return FALSE; /* Incorrect Permission */
}

int
as_define_stack(addrspace *as)
{
    if (as == NULL) {
        LOG_ERROR("Addrspace cannot be null");
        return 1;
    }

    if (as->region_stack) {
        LOG_ERROR("Stack region already defined");
        return 1;
    }

    region *stack = as_create_region(
        PROCESS_STACK_TOP - PAGE_SIZE_4K,
        PAGE_SIZE_4K,
        seL4_CanRead | seL4_CanWrite
    );
    if (stack == NULL) {
        LOG_ERROR("Failed to create stack region");
        return 1;
    }

    as->region_stack = stack;
    return as_add_region(as, stack);
}

int
as_define_heap(addrspace *as, uint32_t heap_loc)
{
    if (as == NULL) {
        LOG_ERROR("Addrspace cannot be null");
        return 1;
    }

    if (as->region_heap) {
        LOG_ERROR("Heap region already defined");
        return 1;
    }

    region *heap = as_create_region(
        heap_loc,
        0,
        seL4_CanRead | seL4_CanWrite
    );
    if (heap == NULL) {
        LOG_ERROR("Failed to create heap region");
        return 1;
    }

    as->region_heap = heap;
    return as_add_region(as, heap);
}
