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
 * Page the frame belonging to this vaddr
 * @param page_id, the process vaddr of the page
 * @param access_type, the type of access for this page (for permissions mapping)
 * @returns 1 on failure, else 0
 */
int page_in(seL4_Word page_id, seL4_Word access_type);

/*
 * Try paging a frame out to disk to make room for a new frame
 * @param[out] vaddr, the sos vaddr of the frame
 * @returns -1 on failure, else the id of the frame
 */
int page_out(seL4_Word *vaddr);

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
