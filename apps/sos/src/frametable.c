/*
 * Frametable Implementation
 * Glenn McGuire and Cameron Lonsdale
 */

#include "frametable.h"

#include <cspace/cspace.h>
#include <sys/panic.h>

#include "ut_manager/ut.h"
#include "mapping.h"

/*
 * Reserve a physical frame
 * @param[out] virtual address of the frame
 */
void
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
    *vaddr = 0xA0000000;
    err = map_page(frame_cap, seL4_CapInitThreadPD, *vaddr, seL4_AllRights, seL4_ARM_Default_VMAttributes);
    conditional_panic(err, "Unable to map page");
}

/*
 * Free a physical frame
 */
void
frame_free(seL4_Word vaddr)
{
    return;
}
