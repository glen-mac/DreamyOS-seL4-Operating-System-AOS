/*
 * Frametable management
 * Glenn McGuire and Cameron Lonsdale
 */

#ifndef _FRAMETABLE_H_
#define _FRAMETABLE_H_

#include <sel4/sel4.h>

/*
 * Initialise the frame table
 * @param low, low address of the UT memory region
 * @param high, high address of the UT memory region
 * @return 0 on success, else 1
 */
int frame_table_init(seL4_Word low, seL4_Word high);

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


#endif /* _MAPPING_H_ */
