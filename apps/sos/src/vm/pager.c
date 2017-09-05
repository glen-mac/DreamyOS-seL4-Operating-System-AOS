/*
 * Demand Pager
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include "pager.h"
#include "frametable.h"
#include "event.h"
#include "picoro.h"

#include <nfs/nfs.h>
#include "network.h"
#include <vm/vm.h>
#include <proc/proc.h>
#include <cspace/cspace.h>

#include <sys/panic.h>
#include <utils/util.h>
#include <string.h>

/* Variable to represent where we are up to in our search for a victim page */
static volatile seL4_Word current_page = 0;
static volatile seL4_Word pager_initialised = FALSE;
static fhandle_t pagefile_handle;

static void pagefile_create_callback(uintptr_t token, enum nfs_stat status, fhandle_t* fh, fattr_t* fattr);

int
init_pager(void)
{
    LOG_INFO("init pager");
    const sattr_t file_attr = {
        .mode = 0664, /* Read write for owner and group, read for everyone */
        // TODO: creation time and stuff
    };

    if (nfs_create(&mnt_point, "pagefile", &file_attr, pagefile_create_callback, (uintptr_t)NULL) != RPC_OK)
        return 1;

    seL4_Word badge;
    seL4_MessageInfo_t message;

    /* Spin until the pager is initialised */
    LOG_INFO("Spinning until pager is initialised");
    while (pager_initialised == FALSE) {
        message = seL4_Wait(_sos_ipc_ep_cap, &badge);
        if (badge & IRQ_EP_BADGE && badge & IRQ_BADGE_NETWORK)
            network_irq();
    }

    LOG_INFO("resuming sos initialisation");

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

    /* Get the page id and pid of the process where this frame is mapped into*/
    seL4_Word pid;
    seL4_Word page_id;
    assert(frame_table_get_page_id(frame_id, &pid, &page_id) == 0);

    LOG_INFO("pid is %d; page_id is %d", pid, page_id);

    /* Get the page cap for the process */
    // TODO: Instead of curproc, index into the proc array with pid
    assert(page_directory_lookup(curproc->p_addrspace->directory, page_id, &pt_cap) == 0);

    LOG_INFO("pt_cap is %d", pt_cap);

    /* Unmap the page from the process so we get a fault access */
    seL4_ARM_Page_Unmap(pt_cap);
    cspace_delete_cap(cur_cspace, pt_cap);

    // 2. Write the page to the pagefile (needs to still be mapped into sos for this)
    // Need to store the cap somewhere aswell
    // Do not need to write it if the page was clean, but we need to handle that bug free

    // 3. Yield until NFS comes back, this is where problems happen. Because we could go back to the event loop and 
    // get a fault handle for that page we are currently evicting. I think we need a big lock around paging.
    // We can only page 1 thing at a time, if we get an event which tries to page something in, we check the lock and 
    // reschedule that coro for later once the lock has been free'd (paged successful to disk for example)
    // yield(NULL);

    // 4. Then need to unmap the vaddr from sos
    // maybe a linked list of mapped address so we can loop and unMap them ?
    // 5. UT Free the memory so we can reuse it for another frame 
    frame_free(frame_id);

    return 0;
}

static void
pagefile_create_callback(uintptr_t token, enum nfs_stat status, fhandle_t* fh, fattr_t* fattr)
{
    LOG_INFO("pagefile created");
    assert(status == NFS_OK);
    memcpy(&pagefile_handle, fh, sizeof(fhandle_t));
    pager_initialised = TRUE;
}
