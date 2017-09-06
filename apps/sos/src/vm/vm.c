/*
 * Virtual memory implementation
 * Cameron Lonsdale & Glenn McGuire
 *
 */

#include "vm.h"

#include "frametable.h"
#include "mapping.h"
#include <proc/proc.h>

#include <utils/arith.h>
#include <utils/util.h>
#include <limits.h>

#define verbose 5
#include <sys/debug.h>
#include <sys/panic.h>

/*
 * Two level page table operations
 */
#define DIRECTORY_SIZE_BITS 10
#define TABLE_SIZE_BITS 10
#define CAPS_INDEX_BITS 12

#define DIRECTORY_OFFSET (seL4_WordBits - DIRECTORY_SIZE_BITS)
#define TABLE_OFFSET (seL4_WordBits - DIRECTORY_SIZE_BITS - TABLE_SIZE_BITS)
#define CAP_OFFSET (seL4_WordBits - CAPS_INDEX_BITS)

#define DIRECTORY_MASK (MASK(DIRECTORY_SIZE_BITS) << DIRECTORY_OFFSET)
#define TABLE_MASK (MASK(TABLE_SIZE_BITS) << TABLE_OFFSET)
#define CAP_MASK (MASK(CAPS_INDEX_BITS) << CAP_OFFSET)

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

static seL4_Word get_fault_status(seL4_Word fault_cause);
static int page_table_is_evicted(seL4_Word page_id);

void 
vm_fault(void)
{
    seL4_MessageInfo_t reply;

    /* Ordering defined by seL4 */
    seL4_Word fault_pc = seL4_GetMR(0);
    seL4_Word fault_addr = seL4_GetMR(1);
    seL4_Word fault_type = seL4_GetMR(2);
    seL4_Word fault_cause = seL4_GetMR(3);

    seL4_CPtr reply_cap = cspace_save_reply_cap(cur_cspace);
    if (reply_cap == CSPACE_NULL) {
        LOG_ERROR("reply cap was null, this should not happen");
        goto fault_error;
    }

    seL4_Word access_type = ACCESS_TYPE(fault_cause);
    seL4_Word fault_status = get_fault_status(fault_cause);

    LOG_INFO("Access type %s, fault_status %d", access_type? "Write": "Read", fault_status);
    LOG_INFO("Fault at 0x%08x, pc = 0x%08x, %s", fault_addr, fault_pc, fault_type ? "Instruction Fault" : "Data fault");

    if (fault_status == PERMISSION_FAULT_PAGE) {
        LOG_ERROR("Incorrect permissions");
        goto fault_error;
    }

    /* If the page is marked as evicted, page it in */
    if (page_table_is_evicted(fault_addr)) {
        if (page_in(fault_addr, access_type) != 0) {
            LOG_ERROR("failed to page in");
            goto fault_error;
        }
        goto thread_restart;
    }

    seL4_Word kvaddr;
    if (vm_map(fault_addr, access_type, &kvaddr) != 0) {
        LOG_ERROR("failed to map in a new page");
        goto fault_error;
    }

    thread_restart:
        reply = seL4_MessageInfo_new(0, 0, 0, 0);
        seL4_Send(reply_cap, reply);
        cspace_free_slot(cur_cspace, reply_cap);
        return;

    fault_error:
        panic("TODO: Kill the process");
}

page_directory *
page_directory_create(void)
{   
    seL4_Word directory_vaddr;
    seL4_Word kernel_cap_table_vaddr;
    seL4_Word frame_id;

    page_directory *top_level;
    if ((top_level = malloc(sizeof(page_directory))) == NULL) {
        LOG_ERROR("Cannot allocate memory for page_directory");
        return (page_directory *)NULL;
    }

    if ((frame_id = frame_alloc(&directory_vaddr)) == -1) {
        LOG_ERROR("Cannot alloc frame for directory");
        free(top_level);
        return (page_directory *)NULL;
    }
    /* Prevent the page table from being paged */
    assert(frame_table_set_chance(frame_id, PINNED) == 0);

    seL4_Word nframes = BIT(seL4_PageDirBits - seL4_PageBits);
    if ((frame_id = multi_frame_alloc(&kernel_cap_table_vaddr, nframes)) == -1) {
        LOG_ERROR("Cannot alloc multi frames for cap table");
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
            LOG_ERROR("Cannot alloc second level");
            return 1;
        }
        assert(frame_table_set_chance(frame_id, PINNED) == 0);

        directory[directory_index] = page_table_vaddr;
    }

    page_table_entry *second_level = (page_table_entry *)directory[directory_index];
    // I TURNED THIS OFF BECAUSE THIS WONT BE NULL FOR EVICTED PAGES
    // IDK IF THIS IS THE BEST THING THOUGH, ID BE MORE CONFIDENT WITH THE ERROR CHECKING
    //assert(second_level[table_index].page == (seL4_CPtr)NULL);
    second_level[table_index].page = cap;

    /* Add the kernel cap to our bookkeeping table if one was given to us */
    if (kernel_cap) {
        seL4_Word index = CAP_INDEX(page_id);

        seL4_CPtr *cap_table = dir->kernel_page_table_caps;
        assert(!cap_table[index]);
        cap_table[index] = cap;
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
        LOG_ERROR("page doesnt exist");
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
        LOG_ERROR("page doesnt exist");
        return 1; 
    }

    seL4_CPtr cap = second_level[table_index].page;

    /* Must be less than, as we use the highest bit to represent evicted or not */
    assert(free_id < MAX_CAP_ID);

    second_level[table_index].page = free_id;
    second_level[table_index].page |= EVICTED_BIT; /* Mark as evicted */

    /* Unmap and delete the cap */
    seL4_ARM_Page_Unmap(cap);
    cspace_delete_cap(cur_cspace, cap);

    return 0;
}

int
vm_translate(seL4_Word vaddr, seL4_Word *sos_vaddr)
{
    int err;

    seL4_Word offset = (vaddr & PAGE_MASK_4K);
    seL4_Word page_id = PAGE_ALIGN_4K(vaddr);
    seL4_ARM_Page page_cap;

    if ((err = page_directory_lookup(curproc->p_addrspace->directory, page_id, &page_cap)) != 0) {
        LOG_ERROR("Mapping not found");
        return err;
    }

    seL4_ARM_Page_GetAddress_t paddr_obj = seL4_ARM_Page_GetAddress(page_cap);
    *sos_vaddr = frame_table_paddr_to_sos_vaddr(paddr_obj.paddr + offset);
    return 0;
}

int
vm_map(seL4_Word vaddr, seL4_Word access_type, seL4_Word *kvaddr)
{
    addrspace *as = curproc->p_addrspace;

    /* Try to expand the stack */
    region *vaddr_region;
    if (as_find_region(as, vaddr, &vaddr_region) != 0 &&
        as_region_collision_check(as, PAGE_ALIGN_4K(vaddr), as->region_stack->end) == 0) {
        as->region_stack->start = PAGE_ALIGN_4K(vaddr);
        LOG_INFO("Extended the stack to %p -> %p", (void *)as->region_stack->start, (void *)as->region_stack->end);
    }
    
    /* Check if address belongs to a region and that region has permissions for the access type */
    if (!as_find_region(as, vaddr, &vaddr_region) == 0 ||
        !as_region_permission_check(vaddr_region, access_type)) {
        LOG_INFO("Incorrect Permissions");
        return 1;
    }

    LOG_INFO("before sos_map_page");

    // TODO: This can fail, we need to send ENOMEM to the process.
    assert(sos_map_page(PAGE_ALIGN_4K(vaddr), as, vaddr_region->permissions, kvaddr) == 0);
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
page_table_is_evicted(seL4_Word page_id)
{
    seL4_CPtr cap;

    if (page_directory_lookup(curproc->p_addrspace->directory, PAGE_ALIGN_4K(page_id), &cap) != 0) {
        LOG_INFO("Page entry doesnt exist, cannot be evicted");
        return FALSE;
    }

    return cap & EVICTED_BIT;
}
