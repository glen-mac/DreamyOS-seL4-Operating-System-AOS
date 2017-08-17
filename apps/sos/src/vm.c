/*
 * Virtual memory implementation
 * Cameron Lonsdale & Glenn McGuire
 *
 */

#include "vm.h"

#define verbose 5
#include <sys/debug.h>
#include <sys/panic.h>
#include <assert.h>
#include <utils/page.h>

#include "vmem_layout.h"
#include "mapping.h"
#include "proc.h"

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
vm_fault(seL4_Word fault_addr, seL4_Word pc, seL4_Word fault_type, seL4_Word fault_cause)
{
    seL4_Word access_type = ACCESS_TYPE(fault_cause);
    seL4_Word fault_status = get_fault_status(fault_cause);
    dprintf(0, "Access type %s, fault_status %d\n", access_type? "Write": "Read", fault_status);
    dprintf(0, "vm fault at 0x%08x, pc = 0x%08x, %s\n", fault_addr, pc, fault_type ? "Instruction Fault" : "Data fault");

    if (fault_status == PERMISSION_FAULT_PAGE)
        panic("Incorrect permissions!");

    seL4_CPtr reply_cap = cspace_save_reply_cap(cur_cspace);
    assert(reply_cap != CSPACE_NULL);

    vaddr_region *region;
    assert(proc_get_region(curproc, fault_addr, &region) == 0);
    assert(has_permissions(access_type, region->permissions));
    if (proc_get_region(curproc, fault_addr, &region) == 0 && has_permissions(access_type, region->permissions)) {
        seL4_Word kvaddr;
        assert(sos_map_page(PAGE_ALIGN_4K(fault_addr), curproc->vroot, region->permissions, &kvaddr) == 0);

        if (region == curproc->region_stack) {
            region->vaddr_start -= PAGE_SIZE_4K;
            dprintf(0, "mapped more of the stack\n");
        }
    }

    seL4_MessageInfo_t reply = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, 0);
    seL4_Send(reply_cap, reply);
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
