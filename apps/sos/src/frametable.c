/*
 * Frametable Implementation
 * Glenn McGuire and Cameron Lonsdale
 */

#include "frametable.h"

#include <assert.h>
#include <strings.h>

#include <cspace/cspace.h>
#include <utils/page.h>
#include <utils/util.h>

#include "vmem_layout.h"
#include "ut_manager/ut.h"
#include "mapping.h"

/* Macros to go from physical address to frame table index and the reverse */
#define ADDR_TO_INDEX(paddr) ((paddr - ut_base) >> seL4_PageBits)
#define INDEX_TO_ADDR(index) (ut_base + (index << seL4_PageBits))

/* Portion of pages to give to SOS applications */
#define FT_PORTION 0.8

/* High and low watermark for number of free frames */
#define HIGH_WATERMARK 64
#define LOW_WATERMARK 32

static void _frame_free(seL4_Word frame_id);
static int retype_and_map(seL4_Word paddr, seL4_Word vaddr, seL4_ARM_Page *frame_cap);

/* The frame table is an array of frame entries */
frame_entry *frame_table = NULL;

/* The base and limit of the untyped memory chunk we manage as part of the frame table */
static seL4_Word ut_base;
static seL4_Word ut_top;

/* Sizing Constraints */
static seL4_Word frame_table_max = 0;
static seL4_Word frame_table_cnt = 0;

/* Global buffer to store free frames */
static uint32_t free_frames[HIGH_WATERMARK];
static uint32_t free_index = 0;

int
frame_table_init(seL4_Word paddr, seL4_Word size_in_bits, seL4_Word low, seL4_Word high)
{
    /* Table already exists */
    if (frame_table)
        return 1;

    /* Save these values to global */
    ut_top = high;
    ut_base = low;

    LOG_INFO("frame_table_init: UT memory starts at %x and ends at %x for a size of %x and %lu pages", ut_base, ut_top, ut_top - ut_base, BYTES_TO_4K_PAGES(ut_top - ut_base));

    /* 
     * Set upper bound on frames that can be allocated.
     * We want to reserve an amount of UT memory for SOS.
     * If SOS can't allocate memory for TCBs, caps, etc, then it will assert failure instead of return failure.
     *
     * We set aside 80% of the memory for user application frames, and 20% of UT for SOS.
     */
    frame_table_max = BYTES_TO_4K_PAGES(ut_top - ut_base) * FT_PORTION;

    seL4_ARM_Page frame_cap;
    seL4_Word vaddr = PHYSICAL_VSTART + paddr;
    frame_table = (frame_entry *)vaddr;

    /*
     * Map our frame table memory into virtual memory.
     * Currently leak capabilities because we do not share or free the frame table.
     */
    for (int i = 0; i < BYTES_TO_4K_PAGES(BIT(size_in_bits)); i++) {
        if (retype_and_map(paddr, vaddr, &frame_cap) != 0)
            return 1;

        vaddr += PAGE_SIZE_4K;
        paddr += PAGE_SIZE_4K;
    }

    return 0;
}

seL4_Word
frame_alloc(seL4_Word *vaddr)
{
    seL4_Word paddr;
    seL4_ARM_Page frame_cap;
    seL4_Word p_id;

    /* Check if frame_table is initialised */
    if (!frame_table)
        goto frame_alloc_error;

    /* Ensure we aren't exceeding limits */
    if (frame_table_cnt >= frame_table_max) {
        LOG_INFO("frame limit exceeded");
        goto frame_alloc_error;
    }

    /* If there are free frames in the buffer */
    if (free_index > 0) {
        p_id = free_frames[free_index];
        free_index--;

        paddr = INDEX_TO_ADDR(p_id);
        *vaddr = PHYSICAL_VSTART + paddr;
        goto frame_alloc_return;
    }
    /* Else, we need to allocate a frame from the UT Memory pool */

    /* Grab a page sized chunk of untyped memory */
    if ((paddr = ut_alloc(seL4_PageBits)) == (seL4_Word)NULL)
        goto frame_alloc_error;

    /* Map the page into SOS virtual address space */
    *vaddr = PHYSICAL_VSTART + paddr;
    if (retype_and_map(paddr, *vaddr, &frame_cap) != 0)
        goto frame_alloc_error;

    /* Offset the paddr to align with our ut block */
    p_id = ADDR_TO_INDEX(paddr);

    /* Store the metadata in the frame table */
    assert(frame_table[p_id].cap == (seL4_ARM_Page)NULL);
    frame_table[p_id].cap = frame_cap;
    frame_table_cnt++;

    /* Zero out the memory and return the id */
    frame_alloc_return:
        bzero((void *)(*vaddr), PAGE_SIZE_4K);
        return p_id;

    /* On error, set the vaddr to null and return -1 */
    frame_alloc_error:
        LOG_ERROR("Unable to allocate frame");
        *vaddr = (seL4_Word)NULL;
        return -1;
}

void
frame_free(seL4_Word frame_id)
{
    /* Check if frame_table is initialised */
    if (!frame_table)
        return;

    /* Check if address is within frame_table bounds */
    if (!ISINRANGE(0, frame_id, ADDR_TO_INDEX(ut_top))) {
        LOG_ERROR("frame_id: %d out of bounds", frame_id);
        return;
    }

    /* Check if capability exists */
    if (!frame_table[frame_id].cap) {
        LOG_ERROR("capability for frame_id: %d does not exist", frame_id);
        return;
    }

    /* If there is space to place the frame into the free frame buffer */
    if (free_index < HIGH_WATERMARK) {
        free_index++;
        free_frames[free_index] = frame_id;
        return;
    }

    /* 
     * Free index if equal to high watermark, we should release the current frame
     * And all frames from low watermark to high watermark
     */
    _frame_free(frame_id);
    for (uint32_t id = LOW_WATERMARK; id < HIGH_WATERMARK; id++)
        _frame_free(free_frames[id]);

    free_index = LOW_WATERMARK - 1;
}


/*
 * Private function to unmap a page, delete the capability and release memory back to UT manager
 * @param frame_id, id of the frame to release. Index into the frame table.
 */
static void 
_frame_free(seL4_Word frame_id)
{
    LOG_INFO("Releasing frame id %d", frame_id);

    seL4_ARM_Page frame_cap = frame_table[frame_id].cap;
    frame_table[frame_id].cap = (seL4_ARM_Page)NULL; /* Error guarding */

    /* Ensure this is a frame we have allocated */
    if (!frame_cap) {
        LOG_INFO("capability for %d does not exist", frame_id);
        return;
    }
    
    /* Unmap the page */
    seL4_ARM_Page_Unmap(frame_cap);
    frame_table_cnt--;

    /* Delete the capability and release the memory back to UT */
    cspace_delete_cap(cur_cspace, frame_cap);
    ut_free(INDEX_TO_ADDR(frame_id), seL4_PageBits);
}

/*
 * Private function to retype and map untyped memory into a virtual address space 
 * @param paddr, the physical address to retype
 * @param vaddr, the virtual adddress to map onto
 * @param[out] frame_cap, the capabilty to the mapped frame
 * @returns 0 on success, else 1
 */
static int
retype_and_map(seL4_Word paddr, seL4_Word vaddr, seL4_ARM_Page *frame_cap)
{
    if (cspace_ut_retype_addr(paddr, seL4_ARM_SmallPageObject, seL4_PageBits, cur_cspace, frame_cap) != 0)
        return 1;

    if (map_page(*frame_cap, seL4_CapInitThreadPD, vaddr, seL4_AllRights, seL4_ARM_Default_VMAttributes) != 0)
        return 1;

    return 0;
}
