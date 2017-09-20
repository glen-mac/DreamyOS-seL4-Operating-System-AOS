/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include "mapping.h"

#define verbose 1
#include <sys/panic.h>
#include <sys/debug.h>
#include <cspace/cspace.h>
#include <utils/page.h>
#include <utils/util.h>
#include <ut_manager/ut.h>

#include <proc/proc.h>
#include <vm/layout.h>
#include <vm/frametable.h>
#include <vm/vm.h>

extern const seL4_BootInfo *_boot_info;

/*
 * Maps a page table into the root servers page directory
 * @param vaddr The virtual address of the mapping
 * @return 0 on success
 */
static int 
_map_page_table(seL4_ARM_PageDirectory pd, seL4_Word vaddr, seL4_CPtr *pt_cap)
{
    seL4_Word pt_addr;
    int err;

    /* Allocate a PT object */
    if ((pt_addr = ut_alloc(seL4_PageTableBits)) == (seL4_Word)NULL) {
        LOG_ERROR("Error allocating PT object");
        return 1;
    }
    /* Create the frame cap */
    if ((err = cspace_ut_retype_addr(pt_addr, seL4_ARM_PageTableObject, seL4_PageTableBits, cur_cspace, pt_cap))) {
        LOG_ERROR("Error retyping the PT object");
        ut_free(pt_addr, seL4_PageTableBits);
        return err;
    }

    /* Tell seL4 to map the PT in for us */
    return seL4_ARM_PageTable_Map(*pt_cap, pd, vaddr, seL4_ARM_Default_VMAttributes);
}

int 
map_page(seL4_CPtr frame_cap, seL4_ARM_PageDirectory pd, seL4_Word vaddr, 
                seL4_CapRights rights, seL4_ARM_VMAttributes attr, seL4_CPtr *pt_cap)
{
    int err;
    *pt_cap = (seL4_CPtr)NULL; /* kernel cap only set if we mapped in a 2nd level page table */

    /* Attempt the mapping */
    if ((err = seL4_ARM_Page_Map(frame_cap, pd, vaddr, rights, attr)) == seL4_FailedLookup) {
        /* Assume the error was because we have no page table */
        if((err = _map_page_table(pd, vaddr, pt_cap)) == 0)
            /* Try the mapping again */
            err = seL4_ARM_Page_Map(frame_cap, pd, vaddr, rights, attr);
    }

    return err;
}

void * 
map_device(void *paddr, int size)
{
    static seL4_Word virt = DEVICE_START;
    seL4_Word phys = (seL4_Word)paddr;
    seL4_Word vstart = virt;

    LOG_INFO("Mapping device memory 0x%x -> 0x%x (0x%x bytes)", phys, vstart, size);
    while (virt - vstart < size) {
        seL4_Error err;
        seL4_ARM_Page frame_cap;
        seL4_CPtr pt_cap;

        // TODO: This a common code block across multiple files, it could be refactored
        /* Retype the untype to a frame */
        err = cspace_ut_retype_addr(phys, seL4_ARM_SmallPageObject, seL4_PageBits, cur_cspace, &frame_cap);
        conditional_panic(err, "Unable to retype device memory");
        
        /* Map in the page */
        err = map_page(frame_cap, seL4_CapInitThreadPD, virt, seL4_AllRights, 0, &pt_cap);
        conditional_panic(err, "Unable to map device");

        /* Next address */
        phys += BIT(seL4_PageBits);
        virt += BIT(seL4_PageBits);
    }
    return (void *)vstart;
}


int
sos_map_page(seL4_Word page_id, addrspace *as, unsigned long permissions, seL4_Word *kvaddr, proc * curproc)
{
    assert(IS_ALIGNED_4K(page_id));

    seL4_Word frame_id = frame_alloc(kvaddr);
    if (frame_id == -1) {
        LOG_ERROR("Frame allocation failed");
        return 1;
    }

    seL4_ARM_Page frame_cap = frame_table_get_capability(frame_id);
    assert(frame_cap);

    /*
     * In order to map the frame into the applications address space, we must first copy the capability
     * Because we have already used the cap to map the frame into SOS's address space.
     */
    seL4_CPtr new_frame_cap = cspace_copy_cap(cur_cspace, cur_cspace, frame_cap, seL4_AllRights);
    if (!new_frame_cap) {
        LOG_ERROR("Error copying the capability");
        frame_free(frame_id);
        return 1;
    }

    seL4_CPtr pt_cap;
    if (map_page(new_frame_cap, as->vspace, page_id, permissions, seL4_ARM_Default_VMAttributes, &pt_cap) != 0) {
        LOG_ERROR("Error mapping page");
        cspace_delete_cap(cur_cspace, new_frame_cap);
        frame_free(frame_id);
        return 1;
    }

    /* Insert the capability into the processes 2-level page table */
    if (page_directory_insert(as->directory, page_id, new_frame_cap, pt_cap) != 0) {
        LOG_ERROR("Error inserting into page directory");
        seL4_ARM_Page_Unmap(new_frame_cap);
        cspace_delete_cap(cur_cspace, new_frame_cap);
        frame_free(frame_id);
        return 1;
    }

    /* For demand paging, we need to know what process vaddr is mapped to this frame in the frame table entry */
    /* TOOD: HACK PID FOR NOW */
    assert(frame_table_set_page_id(frame_id, curproc->pid, page_id) == 0);

    return 0;
}
