/*
 * Demand Pager
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#ifndef _PAGER_H_
#define _PAGER_H_

#include <proc/proc.h>

/* Maximum number of pages in the pagefile */
#define PAGEFILE_MAX_PAGES (80 * BYTES_TO_4K_PAGES(BIT(20))) /* 80 MB pagefile size */

/* Second chance replacement marker */
enum chance_type {
	FIRST_CHANCE, /* One more chance */
	SECOND_CHANCE, /* Can be paged to disk */
	PINNED, /* Cannot be paged */
};

/*
 * Initialise the pager
 * @returns 0 on success, else 1
 */
int init_pager(seL4_Word paddr, seL4_Word size_in_bits);

/*
 * Page the frame belonging to this vaddr
 * @param curproc, the process requesting the page in
 * @param page_id, the vaddr of the page in the process
 * @param access_type, the type of access for this page (for permissions mapping)
 * @returns 0 on success, else 1
 */
int page_in(proc *curproc, seL4_Word page_id, seL4_Word access_type);

/*
 * Try paging a frame out to disk to make room for a new frame
 * @param[out] vaddr, the sos vaddr of the frame
 * @returns id of the frame on success, else -1
 */
int page_out(seL4_Word *vaddr);

/* 
 * Add a pagefile id to the pagefile free list
 * @param pagefile_id, the id of the page in the pagefile
 */
void pagefile_free_add(seL4_CPtr pagefile_id);

#endif /* _PAGER_H_ */
