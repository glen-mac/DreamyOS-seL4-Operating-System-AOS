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
#include <vm/addrspace.h>
#include <vm/frametable.h>
#include <fs/sos_nfs.h>

#include <vm/layout.h>
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
load_segment_into_vspace(proc *curproc, vnode *file, uint64_t offset,
                         unsigned long segment_size, unsigned long file_size,
                         unsigned long dst, unsigned long permissions)
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

    addrspace *as = curproc->p_addrspace;

    /* Add the region to the curproc region list */
    as_add_region(as, as_create_region(dst, segment_size, permissions));

    /* We work a page at a time in the destination vspace. */
    unsigned long pos = 0;
    unsigned long nbytes = 0;
    seL4_Word kdst;
    int err;

    while (pos < segment_size) {
        /* Create a mapping inside curproc, and a frame */
        seL4_Word vpage = PAGE_ALIGN_4K(dst);
        if (sos_map_page(curproc, vpage, permissions, &kdst) != 0)
            return 1;

        /* Now copy our data into the destination vspace. */
        nbytes = PAGE_SIZE_4K - (dst & PAGE_MASK_4K);
        if (pos < file_size) {
            // TODO: PIN FRAME

            /* Read data from file and into frame */
            seL4_Word len = MIN(nbytes, file_size - pos);
            uiovec iov = {
                .uiov_base = (void*)(kdst + (dst & PAGE_MASK_4K)),
                .uiov_len = len,
                .uiov_pos = offset,
            };
            if (sos_nfs_read(file, &iov) != len) {
                LOG_ERROR("Failed to read from file");
                // TODO: UNPIN FRAME
                return 1;
            }

            // TODO: UNPIN FRAME
        }

        /* Not observable to I-cache yet so flush the frame */
        seL4_Word frame_id = frame_table_sos_vaddr_to_index(kdst);
        seL4_Word frame_cap = frame_table_get_capability(frame_id);
        seL4_ARM_Page_Unify_Instruction(frame_cap, 0, PAGE_SIZE_4K);

        pos += nbytes;
        dst += nbytes;
        offset += nbytes;
    }

    return 0;
}

int
elf_load(proc *curproc, char *app_name, uint64_t *elf_pc)
{
    char *source_addr;
    unsigned long flags = 0;
    unsigned long file_size = 0;
    unsigned long segment_size = 0;
    unsigned long vaddr = 0;
    uint64_t offset = 0;
    int num_headers;
    int err;

    LOG_INFO("elf_load(%s)", app_name);

    vnode *file;
    if (sos_nfs_lookup(app_name, FALSE, &file) != 0) {
        LOG_ERROR("Failed to find elf file");
        return 1;
    }

    /* Buffer for elf_header (You can assume the header is less than the page size (4 KiB)) */
    char elf_header[PAGE_SIZE_4K];

    uiovec iov = {
        .uiov_base = elf_header,
        .uiov_len = PAGE_SIZE_4K,
        .uiov_pos = 0,
    };
    if (sos_nfs_read(file, &iov) != PAGE_SIZE_4K) {
        LOG_ERROR("Failed to read from file");
        return 1;
    }


    /* Ensure that the ELF file looks sane. */
    if (elf_checkFile(elf_header))
        return seL4_InvalidArgument;


    num_headers = elf_getNumProgramHeaders(elf_header);
    for (int i = 0; i < num_headers; i++) {
        
        /* Skip non-loadable segments (such as debugging data). */
        if (elf_getProgramHeaderType(elf_header, i) != PT_LOAD)
            continue;

        /* Fetch information about this segment. */
        offset = elf_getProgramHeaderOffset(elf_header, i);
        file_size = elf_getProgramHeaderFileSize(elf_header, i);
        segment_size = elf_getProgramHeaderMemorySize(elf_header, i);
        vaddr = elf_getProgramHeaderVaddr(elf_header, i);
        flags = elf_getProgramHeaderFlags(elf_header, i);

        /* Copy into the address space */
        LOG_INFO("Loading segment %08x-->%08x", (int)vaddr, (int)(vaddr + segment_size));
        if (load_segment_into_vspace(curproc, file, offset, segment_size, file_size, vaddr,
                                       get_sel4_rights_from_elf(flags)) != 0) {
            LOG_ERROR("Failed to load segment");
            return 1;
        }
    }

    *elf_pc = elf_getEntryPoint(elf_header);

    /* Map in the heap region after all other regions were added */
    // TOOD: Might be able to move this into create_proc and just grab the info from the end of LL of regions

    addrspace *as = curproc->p_addrspace;

    assert(vaddr != 0);
    seL4_Word heap_loc = PAGE_ALIGN_4K(vaddr + segment_size + PAGE_SIZE_4K);
    region *heap = as_create_region(heap_loc, 0, seL4_CanRead | seL4_CanWrite);
    as_add_region(as, heap);
    as->region_heap = heap;

    return 0;
}
