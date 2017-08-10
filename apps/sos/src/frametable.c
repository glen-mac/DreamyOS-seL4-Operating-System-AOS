/*
 * Frametable Implementation
 * Glenn McGuire and Cameron Lonsdale
 */

#include "frametable.h"

#include <assert.h>
#include <strings.h>

#include <cspace/cspace.h>
#include <sys/panic.h>

#include "vmem_layout.h"
#include "ut_manager/ut.h"
#include "mapping.h"

#include <stdio.h> // DEBUG

#define PAGE_SIZE (1 << seL4_PageBits)
#define PAGE_FRAME 0xFFFFF000   /* Mask for getting page number from addr */

/*
 * Reserve a physical frame
 * @param[out] virtual address of the frame
 * @returns ID of the frame. Index into the frametable
 */
seL4_Word
frame_alloc(seL4_Word *vaddr)
{
    /* Grab a page sized chunk of untyped memory */
    seL4_Word paddr = ut_alloc(seL4_PageBits);
    conditional_panic(!paddr, "Out of memory - could not allocate frame");

    /* Retype the untyped memory to a frame object */
    seL4_ARM_Page frame_cap;
    int err = cspace_ut_retype_addr(paddr, seL4_ARM_SmallPageObject, seL4_PageBits, cur_cspace, &frame_cap);
    conditional_panic(err, "Failed to retype to a frame object");

    /* Map the page into SOS virtual address space */
    *vaddr = PHYSICAL_VSTART + paddr;
    err = map_page(frame_cap, seL4_CapInitThreadPD, *vaddr, seL4_AllRights, seL4_ARM_Default_VMAttributes);
    conditional_panic(err, "Unable to map page");

    /* Zero out the memory */
    bzero((void *)(*vaddr), PAGE_SIZE);

    return paddr & PAGE_FRAME;

    /* TODO: We need to keep track of frame_cap, seL4_CapInitThreadPD, vaddr and paddr */
}

/*
 * Free a physical frame
 */
void
frame_free(seL4_Word frame_id)
{
    assert(frame_id % PAGE_SIZE == 0);
    // seL4_ARM_Page_Unmap(frame_cap);
    // cspace_delete_cap(frame_cap);
    // ut_free(frame_id, seL4_PageBits);
}
