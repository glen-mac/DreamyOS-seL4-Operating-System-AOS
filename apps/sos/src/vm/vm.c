/*
 * Virtual memory
 * Cameron Lonsdale & Glenn McGuire
 *
 */

#include "vm.h"

#include "frametable.h"
#include "mapping.h"
#include <string.h>
#include <utils/util.h>

/*
 * Two level page table operations
 */
/* Size of the top level in bits */
#define DIRECTORY_SIZE_BITS 10
/* Size of the second level in bits */
#define TABLE_SIZE_BITS 10
/* Size of the kernel cap table in bits */
#define CAPS_INDEX_BITS 12

/* Offset to shift in order to get indexes */
#define DIRECTORY_OFFSET (seL4_WordBits - DIRECTORY_SIZE_BITS)
#define TABLE_OFFSET (seL4_WordBits - DIRECTORY_SIZE_BITS - TABLE_SIZE_BITS)
#define CAP_OFFSET (seL4_WordBits - CAPS_INDEX_BITS)

/* Masks */
#define DIRECTORY_MASK (MASK(DIRECTORY_SIZE_BITS) << DIRECTORY_OFFSET)
#define TABLE_MASK (MASK(TABLE_SIZE_BITS) << TABLE_OFFSET)
#define CAP_MASK (MASK(CAPS_INDEX_BITS) << CAP_OFFSET)

/* Macros to retrurn the index into tables given addresses */
#define CAP_INDEX(x) ((x & CAP_MASK) >> CAP_OFFSET)
#define DIRECTORY_INDEX(x) ((x & DIRECTORY_MASK) >> DIRECTORY_OFFSET)
#define TABLE_INDEX(x) ((x & TABLE_MASK) >> TABLE_OFFSET)

/* Fault handling */
#define INSTRUCTION_FAULT 1
#define DATA_FAULT 0

/* 
 * Architecture specifc interpretation of the the fault register
 * http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.100511_0401_10_en/ric1447333676062.html
 */
#define ACCESS_TYPE_BIT 11
#define ACCESS_TYPE_MASK BIT(ACCESS_TYPE_BIT)
#define ACCESS_TYPE(x) ((x & ACCESS_TYPE_MASK) >> ACCESS_TYPE_BIT)

/* Fault statuses */
#define ALIGNMENT_FAULT 0b000001
#define INSTRUCTION_CACHE_MAINTENANCE_FAULT 0b000100
#define TRANSLATION_FAULT_SECTION 0b000101
#define TRANSLATION_FAULT_PAGE 0b000111
#define PERMISSION_FAULT_SECTION 0b001101
#define PERMISSION_FAULT_PAGE 0b001111

/* Check if the evicted bit is set */
#define IS_EVICTED(x) (x & EVICTED_BIT)

/* Private functions */
static seL4_Word get_fault_status(seL4_Word fault_cause);
static int page_table_is_evicted(proc *curproc, seL4_Word page_id);
static int page_table_destroy(page_table_entry *table);
static int page_destroy(seL4_CPtr page_cap);
static int vm_translate(proc *curproc, seL4_Word vaddr, seL4_Word access_type, seL4_Word *sos_vaddr);

void 
vm_fault(seL4_Word pid)
{
    seL4_MessageInfo_t reply;

    proc *curproc = get_proc(pid);
    assert(curproc != NULL);
    
    /* Mark process as blocked */
    proc_mark(curproc, BLOCKED);
    curproc->blocked_ref += 1;
    
    /* Ordering defined by seL4 */
    seL4_Word fault_pc = seL4_GetMR(0);
    seL4_Word fault_addr = seL4_GetMR(1);
    seL4_Word fault_type = seL4_GetMR(2);
    seL4_Word fault_cause = seL4_GetMR(3);

    /* Save the caller */
    seL4_CPtr reply_cap = cspace_save_reply_cap(cur_cspace);
    assert(reply_cap != CSPACE_NULL);

    seL4_Word access_type = ACCESS_TYPE(fault_cause);
    seL4_Word fault_status = get_fault_status(fault_cause);

    LOG_INFO("PID %d, Access Type %s, Fault status %d, Fault address %p, PC %p, Type %s",
        pid, access_type? "Write": "Read", fault_status, (void *)fault_addr,
        (void *)fault_pc, fault_type ? "Instruction Fault" : "Data fault"
    );

    if (fault_status == PERMISSION_FAULT_PAGE) {
        LOG_ERROR("Incorrect permissions");
        goto fault_error;
    }

    /* If the page is marked as evicted, page it in */
    if (page_table_is_evicted(curproc, fault_addr)) {
        if (page_in(curproc, fault_addr, access_type) != 0) {
            LOG_ERROR("Failed to page in");
            goto fault_error;
        }

        goto thread_restart;
    }

    /* Otherwise, try to create a new mapping for this address */
    seL4_Word kvaddr;
    if (vm_map(curproc, PAGE_ALIGN_4K(fault_addr), access_type, &kvaddr) != 0) {
        LOG_ERROR("Failed to map in new page");
        goto fault_error;
    }

    thread_restart:
        /* Mark process as running */
        proc_mark(curproc, RUNNING);
        curproc->blocked_ref -= 1;

        /* Check if the process was signalled to be killed */
        if (curproc->kill_flag && curproc->blocked_ref == 0) {
            LOG_INFO("%d being killed", curproc->pid);
            proc_delete(curproc);
            return;
        }

        reply = seL4_MessageInfo_new(0, 0, 0, 0);
        seL4_Send(reply_cap, reply);
        cspace_free_slot(cur_cspace, reply_cap);
        return;

    fault_error:
        /* Unblock process */
        proc_mark(curproc, RUNNING);
        curproc->blocked_ref -= 1;

        cspace_free_slot(cur_cspace, reply_cap);
        LOG_INFO("%d being killed", curproc->pid);
        proc_delete(curproc);
        return;
}

page_directory *
page_directory_create(void)
{   
    seL4_Word directory_vaddr;
    seL4_Word kernel_cap_table_vaddr;
    seL4_Word frame_id;

    page_directory *top_level;
    if ((top_level = malloc(sizeof(page_directory))) == NULL) {
        LOG_ERROR("Failed to allocate memory for page directory");
        return NULL;
    }

    /* Allocate the frame for the page directory where 2nd level caps are stored */
    if ((frame_id = frame_alloc(&directory_vaddr)) == -1) {
        LOG_ERROR("Failed to allocate frame for directory");
        free(top_level);
        return NULL;
    }
    /* Prevent the page table from being paged */
    assert(frame_table_set_chance(frame_id, PINNED) == 0);

    /* Allocate multi frame buffer for the kernel cap table */
    seL4_Word nframes = BIT(seL4_PageDirBits - seL4_PageBits);
    if ((frame_id = multi_frame_alloc(&kernel_cap_table_vaddr, nframes)) == -1) {
        LOG_ERROR("Failed to allocate multi frame buffer for cap table");
        frame_free(frame_id);
        free(top_level);
        return (page_directory *)NULL;
    }
    /* Prevent the cap table from being paged */
    for (seL4_Word i = 0; i < nframes; i++)
        assert(frame_table_set_chance(frame_id + i, PINNED) == 0);

    top_level->directory = (seL4_Word *)directory_vaddr;
    top_level->kernel_page_table_caps = (seL4_CPtr *)kernel_cap_table_vaddr;

    return top_level;
}

int
page_directory_destroy(page_directory *dir)
{
    if (dir == NULL) {
        LOG_ERROR("Directory is null");
        return 1;
    }

    if (!dir->kernel_page_table_caps) {
        LOG_ERROR("Kernel page table is null");
        return 1;
    }

    if (dir->directory == NULL) {
        LOG_ERROR("Directory is null");
        return 1;
    }

    /* Free the directory */

    /* Free all second levels in the page table */
    for (seL4_Word second_level = 0; second_level < PAGE_SIZE_4K / sizeof(seL4_Word); ++second_level) {
        if (!dir->directory[second_level])
            continue;

        if (page_table_destroy((struct page_table_entry *)(dir->directory[second_level])) != 0) {
            LOG_ERROR("Failed to destroy page table");
            return 1;
        }
    }

    /* Free the top level */
    frame_free(frame_table_sos_vaddr_to_index((seL4_Word)dir->directory));

    /* Free the hardware page table caps */

    /* Destroy kernel page table caps */
    seL4_Word nframes = BIT(seL4_PageDirBits - seL4_PageBits);
    for (size_t i = 0; i < (PAGE_SIZE_4K * nframes) / sizeof(seL4_CPtr); ++i) {
        if (dir->kernel_page_table_caps[i] && (seL4_ARM_PageTable_Unmap(dir->kernel_page_table_caps[i]) != 0)) {
            LOG_ERROR("Failed to destroy hardware page table");
            return 1;
        }
    }

    /* Since frames are contigous, we can get the start id and then increment */
    for (seL4_Word id = frame_table_sos_vaddr_to_index((seL4_Word)(dir->kernel_page_table_caps)); id < nframes; ++id)
        frame_free(id);

    free(dir);
    return 0;
}

int 
page_directory_insert(page_directory *dir, seL4_Word page_id, seL4_CPtr cap, seL4_CPtr kernel_cap)
{
    seL4_Word frame_id;

    assert(IS_ALIGNED_4K(page_id));

    seL4_Word directory_index = DIRECTORY_INDEX(page_id);
    seL4_Word table_index = TABLE_INDEX(page_id);

    if (!dir || !(dir->directory)) {
        LOG_ERROR("Directory doesnt exist");
        return 1;
    }

    seL4_Word *directory = dir->directory;

    /* Alloc the second level if it doesnt exist */
    if (!directory[directory_index]) {
        LOG_INFO("Creating second level page table at index %d", directory_index);
        seL4_Word page_table_vaddr;
        if ((frame_id = frame_alloc(&page_table_vaddr)) == -1) {
            LOG_ERROR("Failed to allocate second level");
            return 1;
        }
        /* Pin the frame */
        assert(frame_table_set_chance(frame_id, PINNED) == 0);
        directory[directory_index] = page_table_vaddr;
    }

    page_table_entry *second_level = (page_table_entry *)directory[directory_index];

    /* Must be less than, as we use the highest bit to represent evicted or not */
    assert(cap < MAX_CAP_ID);
    assert((cap >> 31) == 0);

    /* Store the cap in the pagetable */
    second_level[table_index].page = cap;

    /* Add the kernel cap to our bookkeeping table if one was given to us */
    if (kernel_cap) {
        seL4_Word index = CAP_INDEX(page_id);
        seL4_CPtr *cap_table = dir->kernel_page_table_caps;
        assert(!cap_table[index]);
        cap_table[index] = kernel_cap;
        LOG_INFO("kernel cap inserted into %u", index);
    }

    return 0;
}

int
page_directory_lookup(page_directory *dir, seL4_Word page_id, seL4_CPtr *cap)
{
    assert(IS_ALIGNED_4K(page_id));

    seL4_Word directory_index = DIRECTORY_INDEX(page_id);
    seL4_Word table_index = TABLE_INDEX(page_id);

    if (!dir || !(dir->directory)) {
        LOG_ERROR("Directory doesnt exist");
        return 1;
    }

    seL4_Word *directory = dir->directory;
    page_table_entry *second_level = (page_table_entry *)directory[directory_index];
    if (!second_level) {
        LOG_ERROR("Second level doesnt exist");
        return 1; 
    }

    if (!second_level[table_index].page) {
        LOG_ERROR("Page doesnt exist");
        return 1; 
    }

    *cap = second_level[table_index].page;
    return 0;
}

int
page_directory_evict(page_directory *dir, seL4_Word page_id, seL4_Word free_id)
{
    assert(IS_ALIGNED_4K(page_id));

    seL4_Word directory_index = DIRECTORY_INDEX(page_id);
    seL4_Word table_index = TABLE_INDEX(page_id);

    if (!dir || !(dir->directory)) {
        LOG_ERROR("Directory doesnt exist");
        return 1;
    }

    seL4_Word *directory = dir->directory;
    page_table_entry *second_level = (page_table_entry *)directory[directory_index];
    if (!second_level) {
        LOG_ERROR("Second level doesnt exist");
        return 1; 
    }

    if (!second_level[table_index].page) {
        LOG_ERROR("Page doesnt exist");
        return 1; 
    }

    seL4_CPtr cap = second_level[table_index].page;

    /* Must be less than, as we use the highest bit to represent evicted or not */
    assert(free_id < MAX_CAP_ID);
    assert((free_id >> 31) == 0);

    second_level[table_index].page = free_id;
    second_level[table_index].page |= EVICTED_BIT; /* Mark as evicted */

    /* Unmap and delete the cap */
    seL4_ARM_Page_Unmap(cap);
    cspace_delete_cap(cur_cspace, cap);

    return 0;
}

seL4_Word
vaddr_to_sos_vaddr(proc *curproc, seL4_Word vaddr, seL4_Word access_type)
{
    seL4_Word offset = (vaddr & PAGE_MASK_4K);
    seL4_Word page_id = PAGE_ALIGN_4K(vaddr);
    seL4_Word sos_vaddr;

    /*
     * Attempt to translate vaddr to kvaddr
     * If it failed, try to map in the addr
     * Then the translation should succeed
     */
    if (vm_translate(curproc, vaddr, access_type, &sos_vaddr) != 0) {
        if (vm_map(curproc, page_id, access_type, &sos_vaddr) != 0) {
            LOG_ERROR("Failed to map in file");
            return (seL4_Word)NULL;
        }
        assert(vm_translate(curproc, vaddr, access_type, &sos_vaddr) == 0);
    } else {
        /* Check address has permission for access type */
        addrspace *as = curproc->p_addrspace;
        region *vaddr_region;
        if (!as_find_region(as, vaddr, &vaddr_region) == 0 ||
            !as_region_permission_check(vaddr_region, access_type)) {
            LOG_INFO("Incorrect Permissions");
            return (seL4_Word)NULL;
        }
    }

    /* Return the sos virtual address */
    seL4_ARM_Page frame_cap = frame_table_get_capability(frame_table_sos_vaddr_to_index(sos_vaddr));
    seL4_ARM_Page_GetAddress_t paddr_obj = seL4_ARM_Page_GetAddress(frame_cap);
    return frame_table_paddr_to_sos_vaddr(paddr_obj.paddr + offset);
}

int
copy_in(proc *curproc, void *dst, seL4_Word vaddr_src, seL4_Word nbytes)
{
    seL4_Word bytes_remaining = nbytes;
    seL4_Word sos_vaddr = (seL4_Word)NULL;

    /* Progressively copy in across page boundaries */
    while (bytes_remaining > 0) {
        seL4_Word bytes_this_round = MIN((PAGE_ALIGN_4K(vaddr_src) + PAGE_SIZE_4K) - vaddr_src, bytes_remaining);
        if ((sos_vaddr = vaddr_to_sos_vaddr(curproc, vaddr_src, ACCESS_READ)) == (seL4_Word)NULL) {
            LOG_ERROR("Failed to translate vaddr to sos vaddr");
            return 1;
        }

        memcpy((void *)dst, (char *)sos_vaddr, bytes_this_round);
        dst += bytes_this_round;
        vaddr_src += bytes_this_round;
        bytes_remaining -= bytes_this_round;
    }

    return 0;
}

int
copy_out(proc *curproc, seL4_Word dst, char *src, seL4_Word nbytes)
{
    seL4_Word bytes_remaining = nbytes;
    seL4_Word sos_vaddr = (seL4_Word)NULL;

    /* Progressively copy out across page boundaries */
    while (bytes_remaining > 0) {
        seL4_Word bytes_this_round = MIN((PAGE_ALIGN_4K(dst) + PAGE_SIZE_4K) - dst, bytes_remaining);
        if ((sos_vaddr = vaddr_to_sos_vaddr(curproc, dst, ACCESS_WRITE)) == (seL4_Word)NULL) {
            LOG_ERROR("Failed to translate vaddr to sos vaddr");
            return 1;
        }

        memcpy((void *)sos_vaddr, src, bytes_this_round);
        dst += bytes_this_round;
        src += bytes_this_round;
        bytes_remaining -= bytes_this_round;
    }

    return 0;
}

int
vm_map(proc *curproc, seL4_Word vaddr, seL4_Word access_type, seL4_Word *kvaddr)
{
    addrspace *as = curproc->p_addrspace;

    /* Try to expand the stack */
    region *vaddr_region;
    if (as_find_region(as, vaddr, &vaddr_region) != 0) {
        if (as_region_collision_check(as, as->region_stack, PAGE_ALIGN_4K(vaddr), as->region_stack->end) == 0) {
            /* Will not collide with another region, but did we try to set the stack backwards? */
            if (PAGE_ALIGN_4K(vaddr) >= as->region_stack->end) {
                LOG_ERROR("Cannot decrease the stack region");
                goto stack_extension_epilogue;
            }

            /* Check the stack extension does not exceed the limit */
            if ((seL4_Word)(as->region_stack->end - PAGE_ALIGN_4K(vaddr)) > RLIMIT_STACK_SZ) {
                LOG_ERROR("Tried to extend size of stack beyond RLIMIT_STACK_SZ");
                return 1;
            }

            as->region_stack->start = PAGE_ALIGN_4K(vaddr);
            LOG_INFO("Extended the stack to %p -> %p", (void *)as->region_stack->start, (void *)as->region_stack->end);
        } else {
            LOG_ERROR("Failed to extend the stack");
        }
    }

    stack_extension_epilogue:
        /* Check if address belongs to a region and that region has permissions for the access type */
        if (!as_find_region(as, vaddr, &vaddr_region) == 0 ||
            !as_region_permission_check(vaddr_region, access_type)) {
            LOG_ERROR("Incorrect Permissions");
            return 1;
        }

        if (sos_map_page(curproc, PAGE_ALIGN_4K(vaddr), vaddr_region->permissions, kvaddr) != 0) {
            LOG_ERROR("Failed to map page into sos");
            return 1;
        }

        return 0;
}

/* The status of the fault is indicated by bits 12, 10 and 3:0 all strung together */
static seL4_Word
get_fault_status(seL4_Word fault_cause)
{
    seL4_Word bit_12 = fault_cause & BIT(12);
    seL4_Word bit_10 = fault_cause & BIT(10);
    seL4_Word lower_bits = fault_cause & (BIT(3) | BIT(2) | BIT(1) | BIT(0));
    return (bit_12 << 5) | (bit_10 << 4) | lower_bits;
}

static int
page_table_is_evicted(proc *curproc, seL4_Word page_id)
{
    seL4_CPtr cap;
    if (page_directory_lookup(curproc->p_addrspace->directory, PAGE_ALIGN_4K(page_id), &cap) != 0) {
        LOG_INFO("Page entry doesnt exist, cannot be evicted");
        return FALSE;
    }

    return IS_EVICTED(cap);
}

unsigned
page_directory_count(proc *curproc)
{
    unsigned pages_count = 0;

    /* If we are counting a zombie process */
    if (!curproc->p_addrspace)
        return pages_count;

    seL4_Word *top_pd = curproc->p_addrspace->directory->directory;
    page_table_entry *sec_pd;
    for (int i = 0; i < PAGE_SIZE_4K / sizeof(seL4_Word); i++) {
        /* If second level, count all second level pages */
        if (top_pd[i]) {
            sec_pd = (page_table_entry *)top_pd[i];
            for (int j = 0; j < PAGE_SIZE_4K / sizeof(seL4_Word); j++) {
                if (sec_pd[j].page)
                    pages_count++;
            }
        }
    }

    return pages_count;
}

/*
 * Destroy a page table
 * @returns 0 on success else 1
 */
static int
page_table_destroy(page_table_entry *table)
{
    for (size_t i = 0; i < PAGE_SIZE_4K / sizeof(seL4_CPtr); ++i) {
        if (table[i].page && (page_destroy(table[i].page) != 0)) {
            LOG_ERROR("Failed to destroy page");
            return 1;
        }
    }

    /* Free the frame backing the page table */
    frame_free(frame_table_sos_vaddr_to_index((seL4_Word)table));
    return 0;
}

/*
 * Destroy a page
 * @returns 0 on success else 1
 */
static int
page_destroy(seL4_CPtr page_cap)
{
    seL4_CPtr pagefile_id = page_cap;
    if (IS_EVICTED(page_cap)) {
        pagefile_id &= (~EVICTED_BIT);
        pagefile_free_add(pagefile_id);
        return 0;
    }

    if (seL4_ARM_Page_Unmap(page_cap) != 0) {
        LOG_ERROR("Failed to unmap page");
        return 1;
    }

    seL4_ARM_Page_GetAddress_t paddr_obj = seL4_ARM_Page_GetAddress(page_cap);
    seL4_Word paddr = paddr_obj.paddr;

    if (cspace_delete_cap(cur_cspace, page_cap) != CSPACE_NOERROR) {
        LOG_ERROR("Failed to delete cap for frame");
        return 1;
    }

    /* Free the frame */
    seL4_Word frame_id = frame_table_sos_vaddr_to_index(frame_table_paddr_to_sos_vaddr(paddr));
    frame_free(frame_id);

    return 0;
}

/*
 * Given a vaddr, translate it to the sos vaddr of the frame 
 * @param vaddr, the process virtual address
 * @param access_type, the type of access to this address (for page in)
 * @param[out] sos_vaddr, the vaddr for the frame so sos can access it
 * @returns 0 on success, else 1
 */
static int
vm_translate(proc *curproc, seL4_Word vaddr, seL4_Word access_type, seL4_Word *sos_vaddr)
{
    seL4_Word offset = (vaddr & PAGE_MASK_4K);
    seL4_Word page_id = PAGE_ALIGN_4K(vaddr);
    seL4_ARM_Page page_cap;

    if (page_table_is_evicted(curproc, page_id)) {
        LOG_INFO("Page is evicted, trying to page in");
        if (page_in(curproc, page_id, access_type) != 0) {
            LOG_ERROR("Failed to page in");
            return 1;
        }
    }

    if (page_directory_lookup(curproc->p_addrspace->directory, page_id, &page_cap) != 0) {
        LOG_ERROR("Failed to find mapping");
        return 1;
    }

    /* Return the sos vaddr of this frame */
    seL4_ARM_Page_GetAddress_t paddr_obj = seL4_ARM_Page_GetAddress(page_cap);
    *sos_vaddr = frame_table_paddr_to_sos_vaddr(paddr_obj.paddr + offset);

    return 0;
}
