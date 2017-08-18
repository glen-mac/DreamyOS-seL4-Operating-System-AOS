/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <sel4/sel4.h>
#include <elf/elf.h>
#include <string.h>
#include <assert.h>
#include <cspace/cspace.h>
#include <utils/util.h>

#include "elf.h"
#include "proc.h"
#include "frametable.h"

#include <vmem_layout.h>
#include <ut_manager/ut.h>
#include <mapping.h>

#define verbose 5
#include <sys/debug.h>
#include <sys/panic.h>

/* Minimum of two values. */
#define MIN(a,b) (((a)<(b))?(a):(b))

#define PAGESIZE              (1 << (seL4_PageBits))
#define PAGEMASK              ((PAGESIZE) - 1)
#define PAGE_ALIGN(addr)      ((addr) & ~(PAGEMASK))
#define IS_PAGESIZE_ALIGNED(addr) !((addr) &  (PAGEMASK))


extern seL4_ARM_PageDirectory dest_as;

/*
 * Convert ELF permissions into seL4 permissions.
 */
static inline seL4_Word get_sel4_rights_from_elf(unsigned long permissions) {
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
 * TODO: Don't keep these pages mapped in
 */
static int load_segment_into_vspace(seL4_ARM_PageDirectory dest_as,
                                    char *src, unsigned long segment_size,
                                    unsigned long file_size, unsigned long dst,
                                    unsigned long permissions) {

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

    /* add the region to the curproc region list */
    proc_add_region(proc_create_region(dst, segment_size, curproc, permissions), curproc);

    /* We work a page at a time in the destination vspace. */
    unsigned long pos = 0;
    unsigned long nbytes = 0;
    seL4_Word kdst;
    int err;

    while (pos < segment_size) {
        seL4_Word vpage = PAGE_ALIGN(dst);

        err = sos_map_page(vpage, dest_as, permissions, &kdst);
        conditional_panic(err, "mapping elf segment failed failed");

        /* Now copy our data into the destination vspace. */
        nbytes = PAGESIZE - (dst & PAGEMASK);
        if (pos < file_size)
            memcpy((void*)kdst, (void*)src, MIN(nbytes, file_size - pos));

        /* Not observable to I-cache yet so flush the frame */
        seL4_Word frame_cap = frame_table_get_capability(frame_table_sos_vaddr_to_index(kdst));
        seL4_ARM_Page_Unify_Instruction(frame_cap, 0, PAGESIZE);

        pos += nbytes;
        dst += nbytes;
        src += nbytes;
    }
    return 0;
}

int elf_load(seL4_ARM_PageDirectory dest_as, char *elf_file) {

    char *source_addr;
    unsigned long flags, file_size, segment_size, vaddr = 0;
    int num_headers;
    int err;
    int i;

    /* Ensure that the ELF file looks sane. */
    if (elf_checkFile(elf_file)){
        return seL4_InvalidArgument;
    }

    num_headers = elf_getNumProgramHeaders(elf_file);
    for (i = 0; i < num_headers; i++) {
        
        /* Skip non-loadable segments (such as debugging data). */
        if (elf_getProgramHeaderType(elf_file, i) != PT_LOAD)
            continue;

        /* Fetch information about this segment. */
        source_addr = elf_file + elf_getProgramHeaderOffset(elf_file, i);
        file_size = elf_getProgramHeaderFileSize(elf_file, i);
        segment_size = elf_getProgramHeaderMemorySize(elf_file, i);
        vaddr = elf_getProgramHeaderVaddr(elf_file, i);
        flags = elf_getProgramHeaderFlags(elf_file, i);

        /* Copy it across into the vspace. */
        dprintf(1, " * Loading segment %08x-->%08x\n", (int)vaddr, (int)(vaddr + segment_size));
        err = load_segment_into_vspace(dest_as, source_addr, segment_size, file_size, vaddr,
                                       get_sel4_rights_from_elf(flags) & seL4_AllRights);
        conditional_panic(err != 0, "Elf loading failed!\n");
    }

    /* map in the heap region after all other regions were added */
    assert(vaddr != 0);
    seL4_Word heap_loc = PAGE_ALIGN(vaddr+PAGESIZE);
    vaddr_region *heap = proc_create_region(heap_loc, 0, curproc, seL4_CanRead | seL4_CanWrite);
    proc_add_region(heap, curproc);
    curproc->region_heap = heap;
    
    return 0;
}
