/*
 * Frametable Implementation
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include "frametable.h"
#include "pager.h"

#include <assert.h>
#include <strings.h>

#include <cspace/cspace.h>
#include <utils/page.h>
#include <utils/util.h>
#include <utils/time.h>

#include <vm/layout.h>
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
static seL4_Word _frame_alloc(seL4_Word *vaddr, seL4_Word nframes);
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

    LOG_INFO("UT memory starts at %x and ends at %x for a size of %x and %lu pages", ut_base, ut_top, ut_top - ut_base, BYTES_TO_4K_PAGES(ut_top - ut_base));

    /* 
     * Set upper bound on frames that can be allocated.
     * We want to reserve an amount of UT memory for SOS.
     * If SOS can't allocate memory for TCBs, caps, etc, then it will assert failure instead of return failure.
     *
     * We set aside 80% of the memory for user application frames, and 20% of UT for SOS.
     */
    frame_table_max = BYTES_TO_4K_PAGES(ut_top - ut_base) * FT_PORTION;
    LOG_INFO("frame_table_max is %d", frame_table_max);

    seL4_ARM_Page frame_cap;
    seL4_Word vaddr = PHYSICAL_VSTART + paddr;
    frame_table = (frame_entry *)vaddr;

    /*
     * Map our frame table memory into virtual memory.
     * Currently leak capabilities because we do not share or free the frame table.
     */
    for (int i = 0; i < BYTES_TO_4K_PAGES(BIT(size_in_bits)); i++) {
        if (retype_and_map(paddr, vaddr, &frame_cap) != 0) {
            LOG_ERROR("Frametable mapping failed");
            return 1;
        }

        vaddr += PAGE_SIZE_4K;
        paddr += PAGE_SIZE_4K;
    }

    return 0;
}

seL4_Word
frame_alloc(seL4_Word *vaddr)
{
    seL4_Word paddr;
    seL4_Word p_id;

    if (!frame_table) {
        LOG_ERROR("frame_table uninitialised");
        goto frame_alloc_error;
    }

    /* Ensure we aren't exceeding limits */
    if (frame_table_cnt >= frame_table_max) {
        LOG_INFO("frame limit reached");
        goto frame_alloc_page;
    }

    /* If there are free frames in the buffer */
    if (free_index > 0) {
        p_id = free_frames[free_index];
        free_index--;

        paddr = INDEX_TO_ADDR(p_id);
        *vaddr = PHYSICAL_VSTART + paddr;
        bzero((void *)(*vaddr), PAGE_SIZE_4K);
        frame_table[p_id].chance = FIRST_CHANCE; /* Reset the chance */
        return p_id;
    }

    /* Else, we need to allocate a frame from the UT Memory pool */
    /* If we are out of memory, try paging to disk */
    if ((p_id = _frame_alloc(vaddr, 1)) != -1)
        return p_id;

    frame_alloc_page:
        LOG_INFO("frame_alloc failed, trying to page");
        return -1; /* TODO TURN PAGING BACK ON AFTER PROC WORKS */
        // if ((p_id = page_out(vaddr)) != -1)
        //     return p_id;

    /* On error, set the vaddr to null and return -1 */
    frame_alloc_error:
        LOG_ERROR("Unable to allocate frame");
        *vaddr = (seL4_Word)NULL;
        return -1;
}

void
frame_free(seL4_Word frame_id)
{
    if (!frame_table) {
        LOG_ERROR("frame_table uninitialised");
        return;
    }

    if (!ISINRANGE(0, frame_id, ADDR_TO_INDEX(ut_top))) {
        LOG_ERROR("frame_id: %d out of bounds", frame_id);
        return;
    }

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

seL4_ARM_Page 
frame_table_get_capability(seL4_Word frame_id)
{
    if (!frame_table) {
        LOG_ERROR("frame_table uninitialised");
        return (seL4_ARM_Page)NULL;
    }

    if (!ISINRANGE(0, frame_id, ADDR_TO_INDEX(ut_top))) {
        LOG_ERROR("frame_id: %d out of bounds", frame_id);
        return (seL4_ARM_Page)NULL;
    }

    return frame_table[frame_id].cap;
}

seL4_Word
frame_table_index_to_sos_vaddr(seL4_Word frame_id)
{
    return frame_table_paddr_to_sos_vaddr(INDEX_TO_ADDR(frame_id));
}

seL4_Word
frame_table_sos_vaddr_to_index(seL4_Word sos_vaddr)
{
    seL4_Word paddr = sos_vaddr - PHYSICAL_VSTART;
    return ADDR_TO_INDEX(paddr);
}

seL4_Word
frame_table_paddr_to_sos_vaddr(seL4_Word paddr)
{
    seL4_Word sos_vaddr = paddr + PHYSICAL_VSTART;
    return sos_vaddr;
}

seL4_Word
multi_frame_alloc(seL4_Word *vaddr, seL4_Word nframes)
{
    /* Check if frame_table is initialised */
    if (!frame_table) {
        LOG_ERROR("frame_table uninitialised");
        *vaddr = (seL4_Word)NULL;
        return -1;
    }

    return _frame_alloc(vaddr, nframes);
}

int
frame_table_get_limits(seL4_Word *lower, seL4_Word *upper)
{
    /* Check if frame_table is initialised */
    if (!frame_table) {
        LOG_ERROR("frame_table uninitialised");
        return -1;
    }

    *lower = 0;
    *upper = ADDR_TO_INDEX(ut_top);
    return 0;
}

int
frame_table_get_chance(seL4_Word frame_id, enum chance_type *chance)
{
    if (!frame_table) {
        LOG_ERROR("frame_table uninitialised");
        return -1;
    }

    if (!ISINRANGE(0, frame_id, ADDR_TO_INDEX(ut_top))) {
        LOG_ERROR("frame_id: %d out of bounds", frame_id);
        return -1;
    }

    if (!frame_table[frame_id].cap) {
        LOG_ERROR("frame is invalid");
        return -1;
    }

    *chance = frame_table[frame_id].chance;
    return 0;
}

int
frame_table_set_chance(seL4_Word frame_id, enum chance_type chance)
{
    if (!frame_table) {
        LOG_ERROR("frame_table uninitialised");
        return -1;
    }

    if (!ISINRANGE(0, frame_id, ADDR_TO_INDEX(ut_top))) {
        LOG_ERROR("frame_id: %d out of bounds", frame_id);
        return -1;
    }

    if (!frame_table[frame_id].cap) {
        LOG_ERROR("frame is invalid");
        return -1;
    }

    frame_table[frame_id].chance = chance;
    return 0;
}

int
frame_table_set_page_id(seL4_Word frame_id, seL4_Word pid, seL4_Word page_id)
{
    if (!frame_table) {
        LOG_ERROR("frame_table uninitialised");
        return -1;
    }

    if (!ISINRANGE(0, frame_id, ADDR_TO_INDEX(ut_top))) {
        LOG_ERROR("frame_id: %d out of bounds", frame_id);
        return -1;
    }

    if (!frame_table[frame_id].cap) {
        LOG_ERROR("frame is invalid");
        return -1;
    }

    /* With this implementation, we cannot support shared memory */
    frame_table[frame_id].pid = pid;
    frame_table[frame_id].page_id = page_id;
    return 0;
}

int
frame_table_get_page_id(seL4_Word frame_id, seL4_Word *pid, seL4_Word *page_id)
{
    if (!frame_table) {
        LOG_ERROR("frame_table uninitialised");
        return -1;
    }

    if (!ISINRANGE(0, frame_id, ADDR_TO_INDEX(ut_top))) {
        LOG_ERROR("frame_id: %d out of bounds", frame_id);
        return -1;
    }

    if (!frame_table[frame_id].cap) {
        LOG_ERROR("frame is invalid");
        return -1;
    }

    *pid = frame_table[frame_id].pid;
    *page_id = frame_table[frame_id].page_id;
    return 0;
}

static seL4_Word
_frame_alloc(seL4_Word *vaddr, seL4_Word nframes)
{
    seL4_Word paddr;
    seL4_ARM_Page frame_cap;
    seL4_Word p_id;

    /* Grab a page sized chunk of untyped memory */
    if ((paddr = ut_alloc(seL4_PageBits + LOG_BASE_2(nframes))) == (seL4_Word)NULL) 
        goto _frame_alloc_error;

    seL4_Word top_frame_id = ADDR_TO_INDEX(paddr);

    *vaddr = PHYSICAL_VSTART + paddr;
    for (int i = 0; i < nframes; i++) {
        if (retype_and_map(paddr, *vaddr + (i * PAGE_SIZE_4K), &frame_cap) != 0)
            goto _frame_alloc_error;

        /* Offset the paddr to align with our ut block */
        p_id = ADDR_TO_INDEX(paddr);

        /* Store the metadata in the frame table */
        assert(frame_table[p_id].cap == (seL4_ARM_Page)NULL);
        assert(frame_table[p_id].chance == FIRST_CHANCE);

        frame_table[p_id].cap = frame_cap;
        frame_table[p_id].chance = FIRST_CHANCE; /* Reset the chance */
        frame_table_cnt++;
        paddr += PAGE_SIZE_4K;
    }

    bzero((void *)(*vaddr), PAGE_SIZE_4K * nframes);
    return top_frame_id;

    /* On error, set the vaddr to null and return -1 */
    _frame_alloc_error:
        LOG_ERROR("Unable to allocate frame");
        *vaddr = (seL4_Word)NULL;
        return -1;
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
    frame_table[frame_id].chance = FIRST_CHANCE; /* Reset the chance */

    if (!frame_cap) {
        LOG_ERROR("capability for %d does not exist", frame_id);
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
    if (cspace_ut_retype_addr(paddr, seL4_ARM_SmallPageObject, seL4_PageBits, cur_cspace, frame_cap) != 0) {
        LOG_ERROR("Retyping to frame failed");
        return 1;
    }

    seL4_CPtr pt_cap;
    if (map_page(*frame_cap, seL4_CapInitThreadPD, vaddr, seL4_AllRights, seL4_ARM_Default_VMAttributes, &pt_cap) != 0) {
        LOG_ERROR("Mapping the page failed");
        return 1;
    }

    return 0;
}
