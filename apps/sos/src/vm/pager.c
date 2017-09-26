/*
 * Demand Pager
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include "pager.h"
#include "frametable.h"
#include "event.h"
#include <coro/picoro.h>

#include <fs/sos_nfs.h>
#include <nfs/nfs.h>
#include "network.h"
#include <vm/vm.h>
#include <proc/proc.h>
#include <cspace/cspace.h>

#include <utils/list.h>
#include <utils/util.h>
#include <utils/page.h>

#include <sys/panic.h>
#include <strings.h>

#define PAGEFILE_MAX_PAGES 5000 // BYTES_TO_4K_PAGES(BIT(31)) // 2 GB pagefile size

/* Variable to represent where we are up to in our search for a victim page */
static volatile seL4_Word current_page = 0;
static volatile seL4_Word pager_initialised = FALSE;
static fhandle_t pagefile_handle;

static list_t *pagefile_free_pages = NULL;
list_t *pagefile_operations = NULL;

static int pagefile_op_compare(void *a, void *b);
static void pagefile_op_append(void *a);
static int pagefile_op_release(void *a);

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

    /* Spin until the pager is initialised */
    LOG_INFO("Spinning until pager is initialised");
    while (pager_initialised == FALSE) {
        seL4_Wait(_sos_ipc_ep_cap, &badge);
        if (badge & IRQ_EP_BADGE && badge & IRQ_BADGE_NETWORK)
            network_irq();
    }

    LOG_INFO("Resuming sos initialisation");

    /* Page file metadata */
    if ((pagefile_free_pages = malloc(sizeof(list_t))) == NULL) {
        LOG_ERROR("failed to malloc free page list");
        return 1;
    }

    /* Page file operations */
    if ((pagefile_operations = malloc(sizeof(list_t))) == NULL) {
        LOG_ERROR("failed to malloc pagefile operations list");
        return 1;
    }

    list_init(pagefile_free_pages);
    list_init(pagefile_operations);

    /* Mark every page on the file as empty */
    LOG_INFO("max pages is %d", PAGEFILE_MAX_PAGES);

    for (size_t id = 0; id < PAGEFILE_MAX_PAGES; ++id) {
        if (list_prepend(pagefile_free_pages, (void *)id) == -1) {
            LOG_ERROR("failed to prepend free page %d", id);
            return 1;
        }
    }

    return 0;
}

int
page_in(proc *curproc, seL4_Word page_id, seL4_Word access_type)
{
    LOG_INFO("paging in %d", page_id);

    seL4_CPtr pagefile_id;
    if (page_directory_lookup(curproc->p_addrspace->directory, PAGE_ALIGN_4K(page_id), &pagefile_id) != 0) {
        LOG_ERROR("lookup of pagefile id failed");
        return 1;
    }

    pagefile_id &= (~EVICTED_BIT);
    LOG_INFO("page is stored at %d", pagefile_id);

    seL4_Word sos_vaddr;

    LOG_INFO("access type is %d", access_type);

    if (vm_map(curproc, page_id, access_type, &sos_vaddr) != 0) {
        LOG_INFO("failed to map in a page");
        return 1;
    }

    /* A page was just pushed to disk, page_id now corresponds to a legit page and 
     * sos_vaddr is where we can access that frame */

    // /* race condition cover */
    // pagefile_op_node * op_node = malloc(sizeof(pagefile_op_node));
    // op_node->pagefile_id = pagefile_id;
    // int add_result = list_action(pagefile_operations, (void *)op_node, pagefile_op_compare, pagefile_op_append);
    // LOG_INFO("Page in: add_result %d", add_result);
    // if (add_result == 1) {
    //      add it to the operations list, and init the waiting coros
    //        list so other coros can append themselves to it to be served later 
    //     list_init(op_node->waiting_coros);
    //     list_append(pagefile_operations, (void *)op_node);
    // } else {
    //     /* so there is a current pagefile operation on the pageid, so we want
    //        to yield so that the operation we are waiting on can resume us */
    //     yield(NULL);
    // }
    // /* race condition cover */

    /* Queue paging operation */
    if (list_append(pagefile_operations, (void *)coro_getcur()) != 0) {
        LOG_ERROR("failed to queue paging operation");
        return -1;
    }
    
    /* If an operation is already happening, yield */
    if (list_length(pagefile_operations) > 1) {
        yield(NULL);
        LOG_INFO("page_in: resumed coro");
    }


    /* Read the page in from the pagefile and into memory */
    vnode handle = {.vn_data = &pagefile_handle};
    size_t bytes_remaining = PAGE_SIZE_4K;
    int result;
    do {
        // Could it be that these values are changing between calling sos_nfs_read and when the callback happens?
        uiovec iov = {
           .uiov_base = (char *)(sos_vaddr + (PAGE_SIZE_4K - bytes_remaining)), // Could it be this?
           .uiov_len = bytes_remaining,
           .uiov_pos = (pagefile_id * PAGE_SIZE_4K) + (PAGE_SIZE_4K - bytes_remaining)
        };

        if ((result = sos_nfs_read(&handle, &iov)) == -1) {
            LOG_ERROR("Error when reading from pagefile");
            return 1;
        }

        char *data_ptr = (char *)(sos_vaddr + (PAGE_SIZE_4K - bytes_remaining));

        bytes_remaining -= result;
    } while (bytes_remaining > 0);

    /* Mark the page as free in the pagefile */
    pagefile_free_add(pagefile_id);
    seL4_ARM_Page_Unify_Instruction(frame_table_get_capability(frame_table_sos_vaddr_to_index(sos_vaddr)), 0, PAGE_SIZE_4K);

    /* Remove finished operation from the queue */
    struct list_node *op = pagefile_operations->head;
    pagefile_operations->head = op->next;
    free(op);

    /* Run the next pagefile operation if any */
    if (!list_is_empty(pagefile_operations))
        resume(pagefile_operations->head->data, NULL);

    // /* release all waiting coros */
    // if (op_node->waiting_coros) {
    //     list_foreach(op_node->waiting_coros, pagefile_op_release);
    //     list_remove_all(op_node->waiting_coros);
    //     list_remove(pagefile_operations, op_node, pagefile_op_compare);        
    // }

    /* Remove finished operation from the queue */

    return 0;
}

int
page_out(seL4_Word *page_id)
{
    /* Queue paging operation */
    if (list_append(pagefile_operations, (void *)coro_getcur()) != 0) {
        LOG_ERROR("failed to queue paging operation");
        return -1;
    }
    
    LOG_INFO("Pageout length: %d", list_length(pagefile_operations));

    /* If an operation is already happening, yield */
    if (list_length(pagefile_operations) > 1) {
        yield(NULL);
        LOG_INFO("page_out: resumed coro");
    }

    if (list_is_empty(pagefile_free_pages)) {
        LOG_ERROR("No more space in the pagefile");
        return -1;
    }

    seL4_Word frame_id = next_victim();
    if (frame_id == -1) {
        LOG_ERROR("Unable to choose a victim");
        return -1; /* We're unable to choose a vicitm, all pages are pinned */
    }

    LOG_INFO("victim frame is %d", frame_id);

    /* Evict the frame from memory and push it to the disk */
    if (evict_frame(frame_id) == -1) {
        LOG_ERROR("Evicting frame failed");
        return -1;
    }

    /* Zero out the frame and return the page_id / frame_id */
    *page_id = frame_table_index_to_sos_vaddr(frame_id);
    bzero((void *)(*page_id), PAGE_SIZE_4K);

    /* Reset the chance on the frame */
    assert(frame_table_set_chance(frame_id, FIRST_CHANCE) == 0);

    /* Remove finished operation from the queue */
    struct list_node *op = pagefile_operations->head;
    pagefile_operations->head = op->next;
    free(op);

    /* Run the next pagefile operation if any */
    if (!list_is_empty(pagefile_operations))
        resume(pagefile_operations->head->data, NULL);

    return frame_id;
}

int
next_victim(void)
{
    seL4_Word lower;
    seL4_Word upper;
    seL4_Word current_old;
    assert(frame_table_get_limits(&lower, &upper) == 0);

    enum chance_type chance;
    seL4_Word starting_page = current_page;
    seL4_Word loops = 0;

    while (TRUE) {
        if (!frame_table_get_capability(current_page))
            goto next;

        assert(frame_table_get_chance(current_page, &chance) == 0);
        LOG_INFO("chance baby %d", current_page);
        switch (chance) {
            /* Increment the chance of the current page and move on */
            case FIRST_CHANCE:
                frame_table_set_chance(current_page, SECOND_CHANCE);
                break;
            
            /* can't touch this page so move on */
            case PINNED:
                goto next;

            /* return current page and set the current as the next */
            case SECOND_CHANCE:
                current_old = current_page;
                current_page = ((current_page + 1) % (upper - lower)) + lower;
                return current_old;
        }
   
        if (current_page == starting_page)
            loops++;

        /* We've looped around twice and have not chosen a page, all the pages are pinned */
        if (loops == 1)
            break;

        next:
            current_page = ((current_page + 1) % (upper - lower)) + lower;
    }

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

    LOG_INFO("Evicting pid is %d; page_id is %p", pid, page_id);

    /* Get the page cap for the process */
    proc *curproc = get_proc(pid);
    if (curproc == NULL) {
        LOG_ERROR("Invalid pid");
        return 1;
    }

    /* Pop the next free page off the list */
    void *free_node = pagefile_free_pages->head;
    seL4_Word pagefile_id = (seL4_Word)pagefile_free_pages->head->data;
    pagefile_free_pages->head = pagefile_free_pages->head->next;
    free(free_node);

    // /* race condition cover */
    // pagefile_op_node *op_node = malloc(sizeof(pagefile_op_node));
    // op_node->pagefile_id = pagefile_id;
    // int add_result = list_action(pagefile_operations, (void *)op_node, pagefile_op_compare, pagefile_op_append);
    // LOG_INFO("evict_frame: add_result %d", add_result);
    // if (add_result == 1) {
    //      add it to the operations list, and init the waiting coros
    //        list so other coros can append themselves to it to be served later 
    //     list_init(op_node->waiting_coros);
    //     list_append(pagefile_operations, (void *)op_node);
    // } else {
    //     /* so there is a current pagefile operation on the pageid, so we want
    //        to yield so that the operation we are waiting on can resume us */
    //     yield(NULL);
    // }
    // /* race condition cover */

    LOG_INFO("Free page in file at %d", pagefile_id);

    assert(page_directory_evict(curproc->p_addrspace->directory, page_id, pagefile_id) == 0);

    LOG_INFO("page entry evicted");

    /* Write the page to disk */
    /* TODO: DONT NEED TO WRITE PAGE IF ITS NOT DIRTY? */
    seL4_Word sos_vaddr = frame_table_index_to_sos_vaddr(frame_id);
    vnode handle = {.vn_data = &pagefile_handle};
    size_t bytes_remaining = PAGE_SIZE_4K;
    int result;
    do {
        uiovec iov = {
           .uiov_base = (char *)(sos_vaddr + (PAGE_SIZE_4K - bytes_remaining)),
           .uiov_len = bytes_remaining,
           .uiov_pos = (pagefile_id * PAGE_SIZE_4K) + (PAGE_SIZE_4K - bytes_remaining)
        };

        LOG_INFO("WRITE: base %p, len %lu, pos %lu", iov.uiov_base, iov.uiov_len, iov.uiov_pos);

        if ((result = sos_nfs_write(&handle, &iov)) == -1) {
            LOG_ERROR("Error when writing to pagefile");
            return 1;
        }

        char *data_ptr = (char *)(sos_vaddr + (PAGE_SIZE_4K - bytes_remaining));
        LOG_INFO("data: %d", *data_ptr);

        LOG_INFO("WRITE: result was %d", result);

        bytes_remaining -= result;

    } while (bytes_remaining > 0);

    // 3. Yield until NFS comes back, this is where problems happen. Because we could go back to the event loop and 
    // get a fault handle for that page we are currently evicting. I think we need a big lock around paging.
    // We can only page 1 thing at a time, if we get an event which tries to page something in, we check the lock and 
    // reschedule that coro for later once the lock has been free'd (paged successful to disk for example)
    // yield(NULL);

    // DONT THINK WE NEED TO DO THIS. THIS IS HANDLED IN THE CALLER FUNCTION
    // 4. Then need to unmap the vaddr from sos
    // maybe a linked list of mapped address so we can loop and unMap them ?
    // 5. UT Free the memory so we can reuse it for another frame 
    // frame_free(frame_id);

    /* release all waiting coros */
    // if (op_node->waiting_coros) {
    //     list_foreach(op_node->waiting_coros, pagefile_op_release);
    //     list_remove_all(op_node->waiting_coros);
    //     list_remove(pagefile_operations, op_node, pagefile_op_compare);        
    // }

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

void
pagefile_free_add(seL4_CPtr pagefile_id) {
    assert(list_prepend(pagefile_free_pages, (void *)pagefile_id) == 0);
}

static int
pagefile_op_compare(void *a, void *b) {
    return (((pagefile_op_node *)a)->pagefile_id != ((pagefile_op_node *)b)->pagefile_id);
}

static void
pagefile_op_append(void *a) {
    pagefile_op_node *cur_node = (pagefile_op_node *)a;
    list_append(cur_node->waiting_coros, (void *)coro_getcur());
}

static int
pagefile_op_release(void *a) {
    coro op_coro = (coro)a;
    resume(op_coro, NULL);
}
