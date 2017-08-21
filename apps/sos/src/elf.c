/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include "elf.h"

#include <sel4/sel4.h>
#include <elf/elf.h>
#include <string.h>
#include <assert.h>
#include <cspace/cspace.h>

#include <utils/arith.h>
#include <utils/util.h>
#include <utils/page.h>

#include "proc.h"
#include "addrspace.h"
#include "frametable.h"

#include <vmem_layout.h>
#include <ut_manager/ut.h>
#include <mapping.h>

#define verbose 5
#include <sys/debug.h>
#include <sys/panic.h>

/*
 * Convert ELF permissions into seL4 permissions.
 * @param permissions
 * @return encoded permissions
 */
static inline seL4_Word
get_sel4_rights_from_elf(unsigned long permissions)
{
    seL4_Word result = 0;

    if (permissions & PF_R)
        result |= seL4_CanRead;
    if (permissions & PF_X)
        result |= seL4_CanRead;
    if (permissions & PF_W)
        result |= seL4_CanWrite;

    return result;
}

/*
 * Inject data into the given vspace.
 */
static int
load_segment_into_vspace(addrspace *as,
                         char *src, unsigned long segment_size,
                         unsigned long file_size, unsigned long dst,
                         unsigned long permissions)
{

    /* Overview of ELF segment loading

       dst: destination base virtual address of the segment being loaded
       segment_size: obvious
       
       So the segment range to "load" is [dst, dst + segment_size).

       The content to load is either zeros or the content of the ELF
       file itself, or both.

       The split between file content and zeros is a follows.

       File content: [dst, dst + file_size)
       Zeros:        [dst + file_size, dst + segment_size)

       Note: if file_size == segment_size, there is no zero-filled region.
       Note: if file_size == 0, the whole segment is just zero filled.

       The code below relies on seL4's frame allocator already
       zero-filling a newly allocated frame.

    */
    
    assert(file_size <= segment_size);

    /* Add the region to the curproc region list */
    as_add_region(as, as_create_region(dst, segment_size, permissions));

    /* We work a page at a time in the destination vspace. */
    unsigned long pos = 0;
    unsigned long nbytes = 0;
    seL4_Word kdst;
    int err;

    while (pos < segment_size) {
        seL4_Word vpage = PAGE_ALIGN_4K(dst);

        err = sos_map_page(vpage, as, permissions, &kdst);
        // TODO: Return error instead of panicing
        conditional_panic(err, "mapping elf segment failed failed");

        /* Now copy our data into the destination vspace. */
        nbytes = PAGE_SIZE_4K - (dst & PAGE_MASK_4K);
        if (pos < file_size)
            memcpy((void*)(kdst + (dst & PAGE_MASK_4K)), (void*)src, MIN(nbytes, file_size - pos));

        /* Not observable to I-cache yet so flush the frame */
        seL4_Word frame_cap = frame_table_get_capability(frame_table_sos_vaddr_to_index(kdst));
        seL4_ARM_Page_Unify_Instruction(frame_cap, 0, PAGE_SIZE_4K);

        pos += nbytes;
        dst += nbytes;
        src += nbytes;
    }

    return 0;
}

int
elf_load(addrspace *as, char *elf_file)
{
    char *source_addr;
    unsigned long flags, file_size, segment_size, vaddr = 0;
    int num_headers;
    int err;

    /* Ensure that the ELF file looks sane. */
    if (elf_checkFile(elf_file))
        return seL4_InvalidArgument;

    num_headers = elf_getNumProgramHeaders(elf_file);
    for (int i = 0; i < num_headers; i++) {
        
        /* Skip non-loadable segments (such as debugging data). */
        if (elf_getProgramHeaderType(elf_file, i) != PT_LOAD)
            continue;

        /* Fetch information about this segment. */
        source_addr = elf_file + elf_getProgramHeaderOffset(elf_file, i);
        file_size = elf_getProgramHeaderFileSize(elf_file, i);
        segment_size = elf_getProgramHeaderMemorySize(elf_file, i);
        vaddr = elf_getProgramHeaderVaddr(elf_file, i);
        flags = elf_getProgramHeaderFlags(elf_file, i);

        /* Copy into the address space */
        // TODO: Should probably return error here instead of panicing.
        LOG_INFO("Loading segment %08x-->%08x", (int)vaddr, (int)(vaddr + segment_size));
        err = load_segment_into_vspace(as, source_addr, segment_size, file_size, vaddr,
                                       get_sel4_rights_from_elf(flags));
        conditional_panic(err != 0, "Elf loading failed!\n");
    }

    /* Map in the heap region after all other regions were added */
    // TOOD: Might be able to move this into create_proc and just grab the info from the end of LL of regions
    assert(vaddr != 0);
    seL4_Word heap_loc = PAGE_ALIGN_4K(vaddr + segment_size + PAGE_SIZE_4K);
    region *heap = as_create_region(heap_loc, 0, seL4_CanRead | seL4_CanWrite);
    as_add_region(as, heap);
    as->region_heap = heap;
    
    return 0;
}
