/*
 * Demand Pager
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include "pager.h"
#include "frametable.h"

#include <sys/panic.h>
#include <utils/util.h>

/* Variable to represent where we are up to in our search for a victim page */
static volatile seL4_Word current_page = 0;

int
init_pager(void)
{
    // Create pagefile

    // Create pagefile metadata

    return 0;
}

int
try_paging(seL4_Word *vaddr)
{
    // Some check to see if our pagefile is full?

    seL4_Word frame_id = next_victim();
    LOG_INFO("victim frame is %d", frame_id);

    evict_frame(frame_id);

    // Frame alloc with the now free memory in the frame table

    *vaddr = (seL4_Word)NULL;
    return -1;
}

int
next_victim(void)
{
    seL4_Word lower;
    seL4_Word upper;
    assert(frame_table_get_limits(&lower, &upper) == 0);

    enum chance_type chance;

    while (TRUE) {
        if (!frame_table_get_capability(current_page))
            goto next;

        assert(frame_table_get_chance(current_page, &chance) == 0);
        switch (chance) {
            case PINNED:
                goto next;

            case FIRST_CHANCE:
                frame_table_set_chance(current_page, SECOND_CHANCE);
                break;

            case SECOND_CHANCE:
                return current_page;
        }
   
        next:
            current_page = ((current_page + 1) % (upper - lower)) + lower;
    }

    panic("shouldnt get here");
    return -1;
}

int
evict_frame(seL4_Word frame_id)
{
    // 1. Unmap from process addrspace so it causes a fault next time
    // We're going to need to store the vaddr in the frame table entry

    // 2. Write the page to the pagefile (needs to still be mapped into sos for this)
    // Need to store the cap somewhere aswell
    // Do not need to write it if the page was clean, but we need to handle that bug free

    // 3. Yield until NFS comes back, this is where problems happen. Because we could go back to the event loop and 
    // get a fault handle for that page we are currently evicting. I think we need a big lock around paging.
    // We can only page 1 thing at a time, if we get an event which tries to page osmething in , we check the lock and 
    // reschedule that coro for later once the lock has been free'd (paged successful to disk for example)

    // 4. Then need to unmap the vaddr from sos
    // maybe a linked list of mapped address so we can loop and unMap them ?

    // 5. UT Free the memory so we can reuse it for another frame 
    return 0;
}