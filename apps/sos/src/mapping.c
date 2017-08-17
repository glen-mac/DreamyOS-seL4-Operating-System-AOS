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

#include <ut_manager/ut.h>
#include "vmem_layout.h"
#include "proc.h"
#include "frametable.h"
#include "pagetable.h"

#define verbose 1
#include <sys/panic.h>
#include <sys/debug.h>
#include <cspace/cspace.h>
#include <utils/page.h>

extern const seL4_BootInfo* _boot_info;


/**
 * Maps a page table into the root servers page directory
 * @param vaddr The virtual address of the mapping
 * @return 0 on success
 */
static int 
_map_page_table(seL4_ARM_PageDirectory pd, seL4_Word vaddr){
    seL4_Word pt_addr;
    seL4_ARM_PageTable pt_cap;
    int err;

    /* Allocate a PT object */
    pt_addr = ut_alloc(seL4_PageTableBits);
    if(pt_addr == 0){
        return !0;
    }
    /* Create the frame cap */
    err =  cspace_ut_retype_addr(pt_addr, 
                                 seL4_ARM_PageTableObject,
                                 seL4_PageTableBits,
                                 cur_cspace,
                                 &pt_cap);
    if(err){
        return !0;
    }
    /* Tell seL4 to map the PT in for us */
    err = seL4_ARM_PageTable_Map(pt_cap, 
                                 pd, 
                                 vaddr, 
                                 seL4_ARM_Default_VMAttributes);
    return err;
}

int 
map_page(seL4_CPtr frame_cap, seL4_ARM_PageDirectory pd, seL4_Word vaddr, 
                seL4_CapRights rights, seL4_ARM_VMAttributes attr){
    int err;

    /* Attempt the mapping */
    err = seL4_ARM_Page_Map(frame_cap, pd, vaddr, rights, attr);
    if(err == seL4_FailedLookup){
        /* Assume the error was because we have no page table */
        err = _map_page_table(pd, vaddr);
        if(!err){
            /* Try the mapping again */
            err = seL4_ARM_Page_Map(frame_cap, pd, vaddr, rights, attr);
        }
    }

    return err;
}

void* 
map_device(void* paddr, int size){
    static seL4_Word virt = DEVICE_START;
    seL4_Word phys = (seL4_Word)paddr;
    seL4_Word vstart = virt;

    dprintf(1, "Mapping device memory 0x%x -> 0x%x (0x%x bytes)\n",
                phys, vstart, size);
    while(virt - vstart < size){
        seL4_Error err;
        seL4_ARM_Page frame_cap;
        /* Retype the untype to a frame */
        err = cspace_ut_retype_addr(phys,
                                    seL4_ARM_SmallPageObject,
                                    seL4_PageBits,
                                    cur_cspace,
                                    &frame_cap);
        conditional_panic(err, "Unable to retype device memory");
        /* Map in the page */
        err = map_page(frame_cap, 
                       seL4_CapInitThreadPD, 
                       virt, 
                       seL4_AllRights,
                       0);
        conditional_panic(err, "Unable to map device");
        /* Next address */
        phys += (1 << seL4_PageBits);
        virt += (1 << seL4_PageBits);
    }
    return (void*)vstart;
}


int
sos_map_page(seL4_Word fault_addr, seL4_ARM_PageDirectory address_space)
{
    seL4_Word page_id = PAGE_ALIGN_4K(fault_addr);
    dprintf(0, "page id is %p\n", page_id);

    seL4_Word frame_vaddr;
    seL4_Word frame_id = frame_alloc(&frame_vaddr);

    seL4_ARM_Page frame_cap = frame_table_get_capability(frame_id);
    assert(frame_cap);

    // seL4_ARM_Page_GetAddress_t addr = seL4_ARM_Page_GetAddress(frame_cap);
    // dprintf(0, "address is %p\n", addr.paddr);

    /*
     * In order to map the frame into the applications address space, we must first copy the capability
     * Because we have already used the cap to map the frame into SOS's address space.
     */
    seL4_CPtr new_frame_cap = cspace_copy_cap(cur_cspace, cur_cspace, frame_cap, seL4_AllRights);
    assert(map_page(new_frame_cap, address_space, page_id, seL4_AllRights, seL4_ARM_Default_VMAttributes) == 0);

    /* Insert the capability into the processes 2-level page table */
    assert(page_directory_insert(curproc->page_directory, page_id, new_frame_cap) == 0);

    return 0;
}
