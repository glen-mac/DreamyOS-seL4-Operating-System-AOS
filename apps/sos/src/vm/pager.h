/*
 * Demand Pager
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#ifndef _PAGER_H_
#define _PAGER_H_

#include <sel4/sel4.h>

enum chance_type {
	FIRST_CHANCE, /* One more chance */
	SECOND_CHANCE, /* Can be paged to disk */
	PINNED, /* Cannot be paged */
};

/*
 * Initialise the pager
 * @returns 0 on success else 1
 */
int init_pager(void);

/*
 * Try paging a frame out to disk to make room for a new frame
 * @param[out] vaddr, the sos vaddr of the frame
 * @returns -1 on failure, else the id of the frame
 */
int try_paging(seL4_Word *vaddr);

/*
 * Find the next page to be evicted
 * @returns -1 on failure, else frame id of the frame
 */
int next_victim(void);

/*
 * Evict a frame from the frame table
 * @param frame_id, the id of the frame
 * @returns 0 on success else -1
 */
int evict_frame(seL4_Word frame_id);

#endif /* _PAGER_H_ */
