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

/* sizing constraints */
static seL4_Word frame_table_max = 0;
static seL4_Word frame_table_cnt = 0;

/* The frame table is an array of frame entries */
frame_entry *frame_table = NULL;

/*
 * Init the frame table
 * @return success if 1, 0 otherwise
 */
int frame_table_init() {
    /* table already exists */
    if (frame_table)
        return 0;
    
    /* find the size of the UT block we have to play with */
    seL4_Word high;
    ut_find_memory(&ut_base, &high);

    dprintf(0, "****** ut memory starts at %x and ends at %x for a size of %x and %d pages\n", ut_base, high, high-ut_base, (high-ut_base)/4096);
    
    /* Set upper bound on frames that can be allocated */
    frame_table_max = ((high - ut_base) / PAGE_SIZE) * 0.9;

    /* Allocate the table with enough pages */
    seL4_Word n_pages = (high - ut_base) / PAGE_SIZE;
    frame_table = (frame_entry *)malloc(sizeof(frame_entry) * n_pages);

    if (!frame_table)
        return 1;

    return 0;
}

/*
 * Reserve a physical frame
 * @param[out] virtual address of the frame
 * @returns ID of the frame. Index into the frametable
 */
seL4_Word
frame_alloc(seL4_Word *vaddr)
{
    seL4_Word paddr;
    seL4_ARM_Page frame_cap;

    /* Check if frame_table is initialised */
    if (!frame_table)
        goto frame_alloc_error;

    /* ensure we aren't exceeding limits */
    if (frame_table_cnt >= frame_table_max)
        goto frame_alloc_error;

    /* Grab a page sized chunk of untyped memory */
    if ((paddr = ut_alloc(seL4_PageBits)) == (seL4_Word)NULL)
        goto frame_alloc_error;

    /* Retype the untyped memory to a frame object */
    if (cspace_ut_retype_addr(paddr, seL4_ARM_SmallPageObject, seL4_PageBits, cur_cspace, &frame_cap) != 0)
        goto frame_alloc_error;

    /* Map the page into SOS virtual address space */
    *vaddr = PHYSICAL_VSTART + paddr;
    if (map_page(frame_cap, seL4_CapInitThreadPD, *vaddr, seL4_AllRights, seL4_ARM_Default_VMAttributes) != 0)
        goto frame_alloc_error;

    /* Zero out the memory */
    bzero((void *)(*vaddr), PAGE_SIZE);

    /* Offset the paddr to align with our ut block */
    seL4_Word p_id = (paddr - ut_base) >> seL4_PageBits;

    /* Store the metadata in the frame table */
    /* TODO: We need to keep track of frame_cap, seL4_CapInitThreadPD, vaddr and paddr */
    frame_table[p_id].cap = frame_cap;
    frame_table_cnt++;

    return p_id;

    /* On error, set the vaddr to null and return -1 */
    frame_alloc_error:
        *vaddr = (seL4_Word)NULL;
        return -1;
}

/*
 * Free a physical frame
 * @param index id into the frame_table
 */
void
frame_free(seL4_Word frame_id)
{
    /* Get the cap from the frame_table */
    seL4_CPtr frame_cap = frame_table[frame_id].cap;

    /* ensure this is a frame we can have allocated */
    if (!frame_cap)
        return;
    
    /* Unmap the page */
    seL4_ARM_Page_Unmap(frame_cap);
    frame_table_cnt--;

    /* Delete the capability and release the memory back to UT */
    cspace_delete_cap(cur_cspace, frame_cap);
    ut_free(ut_base + (frame_id << seL4_PageBits), seL4_PageBits);
}
