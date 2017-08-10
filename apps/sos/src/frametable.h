/*
 * Frametable management
 * Glenn McGuire and Cameron Lonsdale
 */

#ifndef _FRAMETABLE_H_
#define _FRAMETABLE_H_

#include <sel4/sel4.h>

/* individual frame */
typedef struct {
	seL4_CPtr cap;		/* the cap for the frame */
} frame_entry;

/* the frame table is an array of frame entries */
frame_entry *frame_table;

/*
 * Init the frame table
 * @return success if 1, 0 otherwise
 */
int frame_table_init();

/*
 * Reserve a physical frame
 * @param[out] virtual address of the frame
 * @return frame id
 */
seL4_Word frame_alloc(seL4_Word *vaddr);

/*
 * Free a physical frame
 */
void frame_free(seL4_Word vaddr);


#endif /* _MAPPING_H_ */
