/*
 * Virtual memory implementation
 * Cameron Lonsdale & Glenn McGuire
 *
 */

#include "vm.h"

#include <sel4/sel4.h>

#include "vmem_layout.h"
#include "mapping.h"
#include "proc.h"


#define verbose 5
#include <sys/debug.h>
#include <sys/panic.h>
#include <assert.h>
#include <utils/page.h>


#include <strings.h>

#include <sys/panic.h>

#include "ut_manager/ut.h"

#include <cspace/cspace.h>
#include <utils/page.h>
#include <utils/util.h>

#include "frametable.h"

/* Fault handling */

#define INSTRUCTION_FAULT 1
#define DATA_FAULT 0

static int has_permissions(seL4_Word access_type, seL4_Word permissions);

/* 
 * Architecture specifc interpretation of the the fault register
 * http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.100511_0401_10_en/ric1447333676062.html
 */
#define ACCESS_TYPE_MASK (1 << 11)
#define ACCESS_TYPE(x) ((x & ACCESS_TYPE_MASK) >> 11)
#define ACCESS_READ 0
#define ACCESS_WRITE 1

/* Fault statuses */
#define ALIGNMENT_FAULT 0b000001
#define INSTRUCTION_CACHE_MAINTENANCE_FAULT 0b000100
#define TRANSLATION_FAULT_SECTION 0b000101
#define TRANSLATION_FAULT_PAGE 0b000111
#define PERMISSION_FAULT_SECTION 0b001101
#define PERMISSION_FAULT_PAGE 0b001111

static seL4_Word get_fault_status(seL4_Word fault_cause);

void 
vm_fault(void)
{
    /* Ordering defined by the kernel */
    seL4_Word fault_pc = seL4_GetMR(0);
    seL4_Word fault_addr = seL4_GetMR(1);
    seL4_Word fault_type = seL4_GetMR(2);
    seL4_Word fault_cause = seL4_GetMR(3);

    seL4_CPtr reply_cap = cspace_save_reply_cap(cur_cspace);
    assert(reply_cap != CSPACE_NULL);

    seL4_Word access_type = ACCESS_TYPE(fault_cause);
    seL4_Word fault_status = get_fault_status(fault_cause);

    LOG_INFO("Access type %s, fault_status %d", access_type? "Write": "Read", fault_status);
    LOG_INFO("Fault at 0x%08x, pc = 0x%08x, %s", fault_addr, fault_pc, fault_type ? "Instruction Fault" : "Data fault");

    if (fault_status == PERMISSION_FAULT_PAGE)
        panic("TODO: Kill the process; Incorrect permissions!");

    addrspace *as = curproc->p_addrspace;

    region *vaddr_region;
    if (as_find_region(as, fault_addr, &vaddr_region) != 0) {
        /* check if the addr lies between the heap end and stack end */
        // TODO: Refactor this to check the addr doesnt collide with any other regions
        if (fault_addr >= as->region_heap->vaddr_end && fault_addr < as->region_stack->vaddr_start) {
            /* if it is, we need to extend the stack! */
            as->region_stack->vaddr_start = PAGE_ALIGN_4K(fault_addr);
        }
    }
    
    /* This assert should pass given the stack may have been extended */
    if (as_find_region(as, fault_addr, &vaddr_region) == 0 && has_permissions(access_type, vaddr_region->permissions)) {
        seL4_Word kvaddr;
        // TODO: This can fail, we need to send ENOMEM to the process.
        assert(sos_map_page(PAGE_ALIGN_4K(fault_addr), as, vaddr_region->permissions, &kvaddr) == 0);
    }

    seL4_MessageInfo_t reply = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, 0);
    seL4_Send(reply_cap, reply);

    cspace_free_slot(cur_cspace, reply_cap);
}

/* The status of the fault is indicated by bits 12, 10 and 3:0 all strung together */
static seL4_Word
get_fault_status(seL4_Word fault_cause)
{
    seL4_Word bit_12 = fault_cause & (1 << 12);
    seL4_Word bit_10 = fault_cause & (1 << 10);
    seL4_Word lower_bits = fault_cause & ((1 << 3) | (1 << 2) | (1 << 1) | (1 << 0));

    return (bit_12 << 5) | (bit_10 << 4) | lower_bits;
}


static int
has_permissions(seL4_Word access_type, seL4_Word permissions)
{
    if (access_type == ACCESS_READ && (permissions & seL4_CanRead))
        return 1;

    if (access_type == ACCESS_WRITE && (permissions & seL4_CanWrite))
        return 1;

    return 0;
}

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

#define DIRECTORY_INDEX(x) ((x & DIRECTORY_MASK) >> DIRECTORY_OFFSET)
#define TABLE_INDEX(x) ((x & TABLE_MASK) >> TABLE_OFFSET)
#define CAP_INDEX(x) ((x & CAP_MASK) >> CAP_OFFSET)

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

    if (multi_frame_alloc(&kernel_cap_table_vaddr, BIT(seL4_PageDirBits - seL4_PageBits)) == -1) {
        LOG_ERROR("Cannot alloc multi frames for cap table");
        frame_free(frame_id);
        free(top_level);
        return (page_directory *)NULL;
    }

    top_level->directory = (seL4_Word *)directory_vaddr;
    top_level->kernel_page_table_caps = (seL4_Word *)kernel_cap_table_vaddr;

    return top_level;
}

int 
page_directory_insert(page_directory *dir, seL4_Word page_id, seL4_CPtr cap, seL4_CPtr kernel_cap)
{
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
        if (frame_alloc(&page_table_vaddr) == -1)
            return 1;

        directory[directory_index] = page_table_vaddr;
    }

    page_table_entry *second_level = (page_table_entry *)directory[directory_index];
    assert(second_level[table_index].page == (seL4_CPtr)NULL);
    second_level[table_index].page = cap;

    if (kernel_cap) {
        seL4_Word index = CAP_INDEX(page_id);

        seL4_CPtr *cap_table = dir->kernel_page_table_caps;
        assert(!cap_table[index]);
        cap_table[index] = cap;
        LOG_INFO("Kernel Cap inserted into %u", index);
    }

    return 0;
}
