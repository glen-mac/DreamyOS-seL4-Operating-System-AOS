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
#include "mapping.h"
#include "network.h"
#include <string.h>
#include <strings.h>
#include <utils/util.h>
#include <vm/layout.h>

/* Variable to represent where we are up to in our search for a victim page */
static volatile seL4_Word current_page = 0;

/* To spin on when waiting for asynchronous initialisation */
static volatile seL4_Word pager_initialised = FALSE;

/* Handle of the pagefile for NFS operations */
static fhandle_t pagefile_handle;

/* Queue of paging operations */
static list_t *pagefile_operations = NULL;

/* Bit array representing the pagefile */
static int *pagefile_metatable = NULL;

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
init_pager(seL4_Word paddr, seL4_Word size_in_bits)
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

    /* Page file operations */
    if ((pagefile_operations = malloc(sizeof(list_t))) == NULL) {
        LOG_ERROR("Failed to allocate memory for paging operations queue");
        return 1;
    }

    list_init(pagefile_operations);

    /* Initilizer the pagefile metatable */
    seL4_Word vaddr = PHYSICAL_VSTART + paddr;
    pagefile_metatable = (int *)vaddr;
    seL4_ARM_Page frame_cap;

    for (int i = 0; i < BYTES_TO_4K_PAGES(BIT(size_in_bits)); i++) {
        if (cspace_ut_retype_addr(paddr, seL4_ARM_SmallPageObject, seL4_PageBits, cur_cspace, &frame_cap) != 0) {
            LOG_ERROR("Failed to retype frame");
            return 1;
        }

        seL4_CPtr pt_cap;
        if (map_page(frame_cap, seL4_CapInitThreadPD, vaddr, seL4_AllRights, seL4_ARM_Default_VMAttributes, &pt_cap) != 0) {
            LOG_ERROR("Failed to map frame");
            return 1;
        }

        vaddr += PAGE_SIZE_4K;
        paddr += PAGE_SIZE_4K;
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

    LOG_INFO("Paging in %p", (void *)page_id);

    seL4_CPtr pagefile_id;
    if (page_directory_lookup(curproc->p_addrspace->directory, PAGE_ALIGN_4K(page_id), &pagefile_id) != 0) {
        LOG_ERROR("Failed to find page associated with vaddr");
        goto page_in_epilogue;
    }

    assert((pagefile_id & EVICTED_BIT) != 0);
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

    /* Pin the frame until it is claimed */
    assert(frame_table_set_chance(frame_id, PINNED) == 0);

    page_out_epilogue:
        page_gate_close();
        return frame_id;
}

void
pagefile_free_add(seL4_CPtr pagefile_id)
{
    pagefile_metatable[pagefile_id / 32] &= ~(1 << (pagefile_id % 32));
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

    bool starting_page_found = FALSE;
    seL4_Word starting_page = 0;
    seL4_Word looped = FALSE;

    while (TRUE) {
        if (!frame_table_get_capability(current_page))
            goto next;

        /* Set the starting page to be the next valid page */
        if (!starting_page_found) {
            looped = FALSE;
            starting_page = current_page;
        }

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
            if (starting_page_found && (current_page == starting_page)) {
                /* We need this logic in order to ignore the first time this while loop goes around */
                if (looped) {
                    LOG_ERROR("Looped around and didnt select a page, all pages pinned");
                    break;
                } else {
                    looped = TRUE;
                }
            }

            current_page = ((current_page + 1) % (upper - lower)) + lower;
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

    /* Find free spot in metatable */
    seL4_Word pagefile_id = 0;
    for (seL4_Word page = 0; page < PAGEFILE_MAX_PAGES; page++) {
        if ((pagefile_metatable[page / 32] & (1 << (page % 32))) == 0) {
            pagefile_metatable[page / 32] |= 1 << (page % 32);
            pagefile_id = page;
            goto pagefile_space_found;
        }
    }

    /* No space found */
    LOG_ERROR("Failed to find space in the file");
    return -1;

    pagefile_space_found:

    LOG_INFO("Free page in file at %d", pagefile_id);

    if (page_directory_evict(curproc->p_addrspace->directory, page_id, pagefile_id) != 0) {
        LOG_ERROR("Failed to evict directory entry");
        pagefile_free_add(pagefile_id);
        return -1;
    }

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
        pagefile_free_add(pagefile_id);
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
