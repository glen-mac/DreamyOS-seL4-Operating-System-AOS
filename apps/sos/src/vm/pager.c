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
    seL4_Word current_old;
    assert(frame_table_get_limits(&lower, &upper) == 0);

    enum chance_type chance;

    while (TRUE) {
        if (!frame_table_get_capability(current_page))
            goto next;

        assert(frame_table_get_chance(current_page, &chance) == 0);
        switch (chance) {
            
            /* increment the chance of the current page and move on */
            case FIRST_CHANCE:
                frame_table_set_chance(current_page, SECOND_CHANCE);
            
            /* can't touch this page so move on */
            case PINNED:
                goto next;

            /* return current page and set the current as the next */
            case SECOND_CHANCE:
                current_old = current_page;
                current_page = ((current_page + 1) % (upper - lower)) + lower;
                return current_old;
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
    /* cap variable to use for unmapping */
    seL4_CPtr pt_cap; 

    // 1. Unmap from process addrspace so it causes a fault next time
    // We're going to need to store the vaddr in the frame table entry
    /* get the vaddr for the frame entry so we know what to unmap */
    seL4_Word vaddr = frame_table_get_vaddr(frame_id);
    /* set pt_cap with the cap used to map in the frame in the vAS */
    assert(page_directory_lookup(cur_proc->p_addrspace->directory, vaddr, &pt_cap) == 0);
    /* unmap the page from the proc vAS */
    seL4_ARM_Page_Unmap(pt_cap);
    cspace_delete_cap(cur_cspace, pt_cap);

    // 2. Write the page to the pagefile (needs to still be mapped into sos for this)
    // Need to store the cap somewhere aswell
    // Do not need to write it if the page was clean, but we need to handle that bug free

    // 3. Yield until NFS comes back, this is where problems happen. Because we could go back to the event loop and 
    // get a fault handle for that page we are currently evicting. I think we need a big lock around paging.
    // We can only page 1 thing at a time, if we get an event which tries to page something in, we check the lock and 
    // reschedule that coro for later once the lock has been free'd (paged successful to disk for example)
    yield(NULL);

    // 4. Then need to unmap the vaddr from sos
    // maybe a linked list of mapped address so we can loop and unMap them ?
    // 5. UT Free the memory so we can reuse it for another frame 
    frame_free(frame_id);

    return 0;
}
