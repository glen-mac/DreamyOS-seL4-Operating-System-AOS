/*
 * Address spaces
 * 
 * Glenn McGuire & Cameron Lonsdale
 */

#include "addrspace.h"
#include "vm.h"
#include <ut_manager/ut.h>
#include <sel4/sel4.h>
#include <stdlib.h>
#include <string.h>
#include <utils/util.h>
#include <cspace/cspace.h>

addrspace *
as_create(void)
{
    addrspace *as;
    if ((as = malloc(sizeof(addrspace))) == NULL) {
        LOG_ERROR("malloc returned null");
        return NULL;
    }

    as->region_list = NULL;

    /* Create a VSpace */
    if ((as->vspace_addr = ut_alloc(seL4_PageDirBits)) == (seL4_Word)NULL) {
        LOG_ERROR("vspace_addr was null");
        free(as);
        return NULL;
    }

    if (cspace_ut_retype_addr(as->vspace_addr,
                                seL4_ARM_PageDirectoryObject,
                                seL4_PageDirBits,
                                cur_cspace,
                                &(as->vspace))) {
        LOG_ERROR("Error retyping memory");
        ut_free(as->vspace_addr, seL4_PageDirBits);
        free(as);
        return NULL;
    }

    if ((as->directory = page_directory_create()) == NULL) {
        LOG_ERROR("page_directory_create returned null");
        cspace_delete_cap(cur_cspace, as->vspace);
        ut_free(as->vspace_addr, seL4_PageBits);
        free(as);
        return NULL;
    }

    return as;

}

region *
as_create_region(seL4_Word start, seL4_Word size, seL4_Word permissions)
{
    assert(size >= 0);
    region *new_region;
    if ((new_region = (region *)malloc(sizeof(region))) == NULL)
        return NULL;

    new_region->vaddr_start = start;
    new_region->vaddr_end = start + size;
    new_region->permissions = permissions;
    return new_region;
}

int
as_add_region(addrspace *as, region *new_region)
{
    /* If region list is empty */
    if (as->region_list == NULL) {
        as->region_list = new_region;
        goto end_add;
    }

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
as_find_region(addrspace *as, seL4_Word vaddr, region **found_region)
{
    region *curr = as->region_list;
    while (curr != NULL) {
        if (vaddr >= curr->vaddr_start && vaddr < curr->vaddr_end) {
            *found_region = curr;
            return 0;
        }
        curr = curr->next_region;
    }
    return 1;
}
