/*
 * Demand Pager
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include "pager.h"

#include <coro/picoro.h>
#include "event.h"
#include "frametable.h"
#include <fs/sos_nfs.h>
#include "network.h"
#include <string.h>
#include <strings.h>
#include <utils/util.h>

/* Maximum number of pages in the pagefile */
#define PAGEFILE_MAX_PAGES 5000 // BYTES_TO_4K_PAGES(BIT(31)) // 2 GB pagefile size

/* Variable to represent where we are up to in our search for a victim page */
static volatile seL4_Word current_page = 0;

/* To spin on when waiting for asynchronous initialisation */
static volatile seL4_Word pager_initialised = FALSE;

/* Handle of the pagefile for NFS operations */
static fhandle_t pagefile_handle;

/* List of free pages in the pagefile */
static list_t *pagefile_free_pages = NULL;

/* Queue of paging operations */
static list_t *pagefile_operations = NULL;

/* Private functions */
static int next_victim(void);
static int evict_frame(seL4_Word frame_id);
static int page_gate_open(void);
static int page_gate_close(void);
static void pagefile_create_callback(uintptr_t token, enum nfs_stat status, fhandle_t* fh, fattr_t* fattr);

/* Determine if the pager is locked */
static volatile bool global_page_lock = FALSE;

/* Specify an exempt coroutine from the locking mechanism */
static volatile coro lock_free_coro = NULL;

int
init_pager(void)
{
    const sattr_t file_attr = {
        .mode = 0664, /* Read write for owner and group, read for everyone */
    };

    if (nfs_create(&mnt_point, "pagefile", &file_attr, pagefile_create_callback, (uintptr_t)NULL) != RPC_OK) {
        LOG_ERROR("Failed to create pagefile");
        return 1;
    }

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
        LOG_ERROR("Failed to allocate memory for free page list");
        return 1;
    }

    /* Page file operations */
    if ((pagefile_operations = malloc(sizeof(list_t))) == NULL) {
        LOG_ERROR("Failed to allocate memory for paging operations queue");
        return 1;
    }

    list_init(pagefile_free_pages);
    list_init(pagefile_operations);

    /* Mark every page on the file as empty */
    LOG_INFO("Max pages in the pagefile is %d", PAGEFILE_MAX_PAGES);
    for (size_t id = 0; id < PAGEFILE_MAX_PAGES; ++id) {
        if (list_prepend(pagefile_free_pages, (void *)id) == -1) {
            LOG_ERROR("Failed to mark page as free %d", id);
            return 1;
        }
    }

    return 0;
}

int
page_in(proc *curproc, seL4_Word page_id, seL4_Word access_type)
{
    int result = 1;

    /* Pass through the gate, or wait */
    if (page_gate_open() != 0) {
        LOG_ERROR("Failed to pass through the paging gate");
        return 1;
    }

    LOG_INFO("Paging in %lu", page_id);

    seL4_CPtr pagefile_id;
    if (page_directory_lookup(curproc->p_addrspace->directory, PAGE_ALIGN_4K(page_id), &pagefile_id) != 0) {
        LOG_ERROR("Failed to find page associated with vaddr");
        goto page_in_epilogue;
    }

    pagefile_id &= (~EVICTED_BIT);
    LOG_INFO("Page is stored at entry %lu in the pagefile", pagefile_id);

    /*
     * Turn off the locking mechanism for paging until this function returns.
     * The reason being is that vm_map might call page_out, and we want that operation to
     * occur immediately, rather than be queued for later
     */
    seL4_Word sos_vaddr;
    global_page_lock = FALSE;
    lock_free_coro = coro_getcur();
    if (vm_map(curproc, page_id, access_type, &sos_vaddr) != 0) {
        LOG_ERROR("Failed to map in the page");
        global_page_lock = TRUE;
        lock_free_coro = NULL;
        goto page_in_epilogue;
    }

    global_page_lock = TRUE;
    lock_free_coro = NULL;

    /* A page was just pushed to disk, page_id now corresponds to a legit page and 
     * sos_vaddr is where we can access that frame */

    /* Read the page in from the pagefile and into memory */
    vnode handle = {.vn_data = &pagefile_handle};
    uiovec iov = {
       .uiov_base = (char *)sos_vaddr,
       .uiov_len = PAGE_SIZE_4K,
       .uiov_pos = pagefile_id * PAGE_SIZE_4K
    };

    /* sos_nfs_read gauruntees entire data is read */
    if (sos_nfs_read(&handle, &iov) == -1) {
        LOG_ERROR("Failed to read from pagefile");
        goto page_in_epilogue;
    }

    /* Mark the page as free in the pagefile */
    pagefile_free_add(pagefile_id);
    /* Flush the cache in case this page was instruction data */
    seL4_ARM_Page_Unify_Instruction(frame_table_get_capability(frame_table_sos_vaddr_to_index(sos_vaddr)), 0, PAGE_SIZE_4K);

    result = 0;
    page_in_epilogue:
        /* Exit through the gate */
        page_gate_close();
        return result;
}

int
page_out(seL4_Word *page_id)
{
    int frame_id = -1;

    if (page_gate_open() != 0) {
        LOG_ERROR("Failed to pass through the paging gate");
        return -1;
    }

    if (list_is_empty(pagefile_free_pages)) {
        LOG_ERROR("No more space in the pagefile");
        goto page_out_epilogue;
    }

    if ((frame_id = next_victim()) == -1) {
        LOG_ERROR("Failed to select a victim frame");
        goto page_out_epilogue; /* We're unable to choose a vicitm, all pages are pinned */
    }

    LOG_INFO("Paging out %d", frame_id);

    /* Evict the frame from memory and push it to the disk */
    if (evict_frame(frame_id) == -1) {
        LOG_ERROR("Failed to evict frame");
        frame_id = -1;
        goto page_out_epilogue;
    }

    /* Zero out the frame and return the page_id / frame_id */
    *page_id = frame_table_index_to_sos_vaddr(frame_id);
    bzero((void *)(*page_id), PAGE_SIZE_4K);

    /* Reset the chance on the frame */
    assert(frame_table_set_chance(frame_id, FIRST_CHANCE) == 0);

    page_out_epilogue:
        page_gate_close();
        return frame_id;
}

void
pagefile_free_add(seL4_CPtr pagefile_id)
{
    assert(list_prepend(pagefile_free_pages, (void *)pagefile_id) == 0);
}

/*
 * Find the next page to be evicted
 * @returns -1 on failure, else frame id of the frame
 */
static int
next_victim(void)
{
    seL4_Word lower;
    seL4_Word upper;
    seL4_Word current_old;
    assert(frame_table_get_limits(&lower, &upper) == 0);

    enum chance_type chance;
    //seL4_Word starting_page = current_page;
    //seL4_Word loops = 0;

    while (TRUE) {
        if (!frame_table_get_capability(current_page))
            goto next;

        assert(frame_table_get_chance(current_page, &chance) == 0);
        switch (chance) {
            /* Increment the chance of the current page and move on */
            case FIRST_CHANCE:
                frame_table_set_chance(current_page, SECOND_CHANCE);
                goto next;
            
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
            // TODO I SWEAR YOU CAN FIX THIS
            // printf("current_page incremented to %d\n", current_page);
            /* We've looped around twice and have not chosen a page, all the pages are pinned */
            // if (current_page == starting_page) {
            //     printf("current page %d, starting page %d\n", current_page, starting_page);
            //     break;
            // }
    }

    return -1;
}

/*
 * Evict a frame from the frame table
 * @param frame_id, the id of the frame
 * @returns 0 on success else -1
 */
static int
evict_frame(seL4_Word frame_id)
{
    /* Get the page id and pid of the process where this frame is mapped into*/
    seL4_Word pid;
    seL4_Word page_id;
    assert(frame_table_get_page_id(frame_id, &pid, &page_id) == 0);

    LOG_INFO("Evicting frame belonging to process %d at adddress %p", pid, (void *)page_id);

    /* Get the page cap for the process */
    proc *curproc = get_proc(pid);
    if (curproc == NULL) {
        LOG_ERROR("Invalid pid");
        return -1;
    }

    /* Pop the next free page off the list */
    void *free_node = pagefile_free_pages->head;
    seL4_Word pagefile_id = (seL4_Word)pagefile_free_pages->head->data;
    pagefile_free_pages->head = pagefile_free_pages->head->next;
    free(free_node);

    LOG_INFO("Free page in file at %d", pagefile_id);

    assert(page_directory_evict(curproc->p_addrspace->directory, page_id, pagefile_id) == 0);

    /* Write the page to disk */
    seL4_Word sos_vaddr = frame_table_index_to_sos_vaddr(frame_id);
    vnode handle = {.vn_data = &pagefile_handle};
    uiovec iov = {
       .uiov_base = (char *)sos_vaddr,
       .uiov_len = PAGE_SIZE_4K,
       .uiov_pos = pagefile_id * PAGE_SIZE_4K
    };

    /* sos_nfs_write gauruntees all data is written succesfully */
    if (sos_nfs_write(&handle, &iov) == -1) {
        LOG_ERROR("Failed to write to the pagefile");
        return -1;
    }

    return 0;
}

/*
 * Gate to enforce a single page operation at a time
 * A coroutine will pass through the gate if the gate is open
 * But waits if it is not
 * @returns 0 on success, else 1
 */
static int
page_gate_open(void)
{
    /* If an operation is already happening, yield */
    if (global_page_lock) {
        if (list_append(pagefile_operations, (void *)coro_getcur()) != 0) {
            LOG_ERROR("Failed to queue paging operation");
            return 1;
        }

        /* Wait until it is our turn to page */
        yield(NULL);
    }

    global_page_lock = TRUE;
    return 0;
}

/*
 * Gate to signify a finished paging operation
 * Resumes the next operation in the queue
 * If the calling coro is lock free, it passes through the gate without resuming 
 * another coroutine
 * @returns 0 on success, else 1
 */
static int
page_gate_close(void)
{
    /* Exempt coroutines dont need to do this logic */
    if (lock_free_coro == coro_getcur())
        return 0;

    /* Run the next pagefile operation if any */
    if (!list_is_empty(pagefile_operations)) {
        /* Remove finished operation from the queue */
        struct list_node *op = pagefile_operations->head;
        pagefile_operations->head = op->next;

        resume(op->data, NULL);
        free(op);
    } else {
        /* No running operations, gate can be opened */
        global_page_lock = FALSE;
    }

    return 0;
}

static void
pagefile_create_callback(uintptr_t token, enum nfs_stat status, fhandle_t* fh, fattr_t* fattr)
{
    assert(status == NFS_OK);
    memcpy(&pagefile_handle, fh, sizeof(fhandle_t));
    pager_initialised = TRUE;
}
