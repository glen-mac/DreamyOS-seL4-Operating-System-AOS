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
#include <sys/debug.h>
#define verbose 5

#define PAGE_SIZE (1 << seL4_PageBits)
#define PAGE_FRAME 0xFFFFF000   /* Mask for getting page number from addr */
#define ADDR_TO_INDEX(x) ((x & PAGE_FRAME) >> seL4_PageBits)

static seL4_Word ut_base;   /* the base of the UT chunk we reference from */

/*
 * Init the frame table
 * @return success if 1, 0 otherwise
 */
int frame_table_init() {
    /* find the size of the UT block we have to play with */
    seL4_Word high;
    ut_find_memory(&ut_base, &high);

    dprintf(0, "****** ut memory starts at %x and ends at %x for a size of %x and %d pages\n", ut_base, high, high-ut_base, (high-ut_base)/4096);

    /* allocate the table with enough pages */
    seL4_Word n_pages = (high - ut_base) / 4096;
    frame_table = (frame_entry *)malloc(sizeof(frame_entry)*n_pages);

    return (frame_table != NULL);
}

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

    /* offset the paddr to align with our ut block */
    seL4_Word p_id = (paddr - ut_base) >> seL4_PageBits;

    /* Store the metadata in the frame table */
    frame_table[p_id].cap = frame_cap;

    return p_id;

    /* TODO: We need to keep track of frame_cap, seL4_CapInitThreadPD, vaddr and paddr */
}

/*
 * Free a physical frame
 */
void
frame_free(seL4_Word frame_id)
{
    assert(frame_id % PAGE_SIZE == 0);
    
    /* get the cap from the frame_table */
    seL4_CPtr frame_cap = frame_table[frame_id].cap;

    /* clean stuff */
    seL4_ARM_Page_Unmap(frame_cap);
    cspace_delete_cap(cur_cspace, frame_cap);
    ut_free(frame_id << 12, seL4_PageBits);
}
