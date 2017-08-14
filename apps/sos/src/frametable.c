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

#include <utils/page.h>
#include <utils/builtin.h>
#include <utils/math.h>

#include <stdio.h>
#include <sys/debug.h>
#define verbose 5

#define PAGE_SIZE (1 << seL4_PageBits)
#define ADDR_TO_INDEX(paddr) ((paddr - ut_base) >> seL4_PageBits)
#define INDEX_TO_ADDR(index) (ut_base + (index << seL4_PageBits))

#define FT_PORTION 0.8

#define HIGH_WATERMARK 64
#define LOW_WATERMARK 32

static void _frame_free(seL4_Word frame_id);

/* The frame table is an array of frame entries */
frame_entry *frame_table = NULL;

static seL4_Word ut_base; /* The base of the UT chunk we reference from */
static seL4_Word ut_top; /* The top of the UT chunk we reference from */

/* Sizing Constraints */
static seL4_Word frame_table_max = 0;
static seL4_Word frame_table_cnt = 0;

/* Global buffer to store free frames */
static uint32_t free_frames[HIGH_WATERMARK];
static uint32_t free_index = 0;

/*
 * Initialise the frame table
 * @param low, low address of the UT memory region
 * @param high, high address of the UT memory region
 * @return 0 on success, else 1
 */
int
frame_table_init(seL4_Word paddr, seL4_Word size_in_bits, seL4_Word low, seL4_Word high)
{
    /* Table already exists */
    if (frame_table)
        return 1;

    /* Save these values to global */
    ut_top = high;
    ut_base = low;
    
    /* find the size of the UT block we have to play with */
    dprintf(0, "frame_table_init: UT memory starts at %x and ends at %x for a size of %x and %d pages\n", ut_base, high, high-ut_base, (high-ut_base)/4096);

    /* 
     * Set upper bound on frames that can be allocated 
     * We want to reserve an amount of UT memory for SOS.
     * If SOS can't allocate memory for TCBs, caps, etc, then it will assert failure instead of return failure.
     *
     * We set aside 80% of the memory for user application frames, and 20% of UT for SOS.
     */
    frame_table_max = ((ut_top - ut_base) / PAGE_SIZE) * FT_PORTION;

    seL4_ARM_Page frame_cap;
    seL4_Word vaddr = PHYSICAL_VSTART + paddr;
    frame_table = (frame_entry *)vaddr;

    /* Map our frame table memory into virtual memory */
    for (int i = 0; i < BYTES_TO_4K_PAGES(BIT(size_in_bits)); i++) {
        if (cspace_ut_retype_addr(paddr, seL4_ARM_SmallPageObject, seL4_PageBits, cur_cspace, &frame_cap) != 0)
            return 1;
        
        if (map_page(frame_cap, seL4_CapInitThreadPD, vaddr, seL4_AllRights, seL4_ARM_Default_VMAttributes) != 0)
            return 1;

        vaddr += PAGE_SIZE_4K;
        paddr += PAGE_SIZE_4K;
    }

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
    seL4_Word p_id;

    /* Check if frame_table is initialised */
    if (!frame_table)
        goto frame_alloc_error;

    /* Ensure we aren't exceeding limits */
    if (frame_table_cnt >= frame_table_max)
        goto frame_alloc_error;

    /* If there are free frames in the buffer */
    if (free_index > 0) {
        p_id = free_frames[free_index];
        free_index--;

        paddr = INDEX_TO_ADDR(p_id);
        *vaddr = PHYSICAL_VSTART + paddr;
        bzero((void *)(*vaddr), PAGE_SIZE);

        return p_id;
    }

    /* Else, we need to allocate a frame from the UT Memory pool */

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

    /* Offset the paddr to align with our ut block */
    p_id = ADDR_TO_INDEX(paddr);

    /* Store the metadata in the frame table */
    assert(frame_table[p_id].cap == (seL4_CPtr)NULL);
    frame_table[p_id].cap = frame_cap;
    frame_table_cnt++;

    /* Zero out the memory */
    bzero((void *)(*vaddr), PAGE_SIZE);

    return p_id;

    /* On error, set the vaddr to null and return -1 */
    frame_alloc_error:
        *vaddr = (seL4_Word)NULL;
        return -1;
}

/*
 * Free an allocated frame
 * @param frame_id of the frame to be freed.
 */
void
frame_free(seL4_Word frame_id)
{
    /* Check if frame_table is initialised */
    if (!frame_table)
        return;

    /* Check if address is within frame_table bounds */
    if (frame_id < 0 || frame_id > ADDR_TO_INDEX(ut_top))
        return;

    /* If there is space to place the frame into the free frame buffer */
    if (free_index < HIGH_WATERMARK) {
        free_index++;
        free_frames[free_index] = frame_id;
        return;
    }

    dprintf(0, "Releasing frames from %d to %d watermark\n", LOW_WATERMARK, HIGH_WATERMARK);

    /* Free index if equal to high watermark, we should release the current frame
     * And all frames from low watermark to high watermark */
    _frame_free(frame_id);
    for (uint32_t id = LOW_WATERMARK; id < HIGH_WATERMARK; id++)
        _frame_free(free_frames[id]);

    free_index = LOW_WATERMARK - 1;
}


void 
_frame_free(seL4_Word frame_id)
{
    seL4_CPtr frame_cap = frame_table[frame_id].cap;
    frame_table[frame_id].cap = (seL4_CPtr)NULL; /* Error guarding */

    /* Ensure this is a frame we have allocated */
    if (!frame_cap)
        return;
    
    /* Unmap the page */
    seL4_ARM_Page_Unmap(frame_cap);
    frame_table_cnt--;

    /* Delete the capability and release the memory back to UT */
    cspace_delete_cap(cur_cspace, frame_cap);
    ut_free(INDEX_TO_ADDR(frame_id), seL4_PageBits);
}
