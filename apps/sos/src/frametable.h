/*
 * Frametable management
 * Glenn McGuire and Cameron Lonsdale
 */

#ifndef _FRAMETABLE_H_
#define _FRAMETABLE_H_

#include <sel4/sel4.h>

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
