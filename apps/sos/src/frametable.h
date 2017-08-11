/*
 * Frametable management
 * Glenn McGuire and Cameron Lonsdale
 */

#ifndef _FRAMETABLE_H_
#define _FRAMETABLE_H_

#include <sel4/sel4.h>

// TODO: DOES THIS BELONG IN FRAMETABLE.C?

/* Individual frame */
typedef struct {
    seL4_CPtr cap; /* the cap for the frame */
} frame_entry;

/*
 * Initialise the frame table
 * @return 0 on success, else 1
 */
int frame_table_init();

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
