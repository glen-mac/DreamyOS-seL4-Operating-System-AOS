/*
 * Frametable
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#ifndef _FRAMETABLE_H_
#define _FRAMETABLE_H_

#include <sel4/sel4.h>

/* Individual frame */
typedef struct {
    seL4_CPtr cap; /* The cap for the frame */
} frame_entry;

/*
 * Initialise the frame table
 * @param paddr, the physical address of the frame_table
 * @param size_in_bits, the size in bits of the frame_table
 * @param low, low address of the UT memory region
 * @param high, high address of the UT memory region
 * @return 0 on success, else 1
 */
int frame_table_init(seL4_Word paddr, seL4_Word size_in_bits, seL4_Word low, seL4_Word high);

/*
 * Reserve a physical frame
 * @param[out] virtual address of the frame
 * @returns ID of the frame. Index into the frametable
 */
seL4_Word frame_alloc(seL4_Word *vaddr);

/*
 * Free an allocated frame
 * @param frame_id of the frame to be freed.
 */
void frame_free(seL4_Word vaddr);

/*
 * Return the capabilty for a frame
 * @param frame_id, id of the frame to lookup
 * @return capability of the frame, NULL on error.
 */
seL4_ARM_Page frame_table_get_capability(seL4_Word frame_id);

/*
 * Given the SOS vaddr of the frame, return its index
 * @param sos_vaddr, virtual address of the frame inside SOS address space
 * @return id that corresponds to that vaddr
 */
seL4_Word frame_table_sos_vaddr_to_index(seL4_Word sos_vaddr);

/*
 * Translate physical address to sos virtual address
 * @param paddr, the physical address
 * @returns sos virtual address
 */
seL4_Word frame_table_paddr_to_sos_vaddr(seL4_Word paddr);

/*
 * Allocate multiple contiguous frames
 * @param[out] kernel vaddr to access the memory
 * @param nframes, number of frames to allocate
 * @return frame id of the starting first frame, -1 if failed.
 */
seL4_Word multi_frame_alloc(seL4_Word *vaddr, seL4_Word nframes);

#endif /* _FRAMETABLE_H_ */
