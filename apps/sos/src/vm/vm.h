/*
 * Virtual memory interface
 * Cameron Lonsdale & Glenn McGuire
 *
 */

#ifndef _VM_H_
#define _VM_H_

#include <sel4/sel4.h>

#define ACCESS_READ 0
#define ACCESS_WRITE 1

/*
 * Two level page table operations
 */
#define DIRECTORY_SIZE_BITS 10
#define TABLE_SIZE_BITS 10
#define CAPS_INDEX_BITS 12

#define DIRECTORY_OFFSET (seL4_WordBits - DIRECTORY_SIZE_BITS)
#define TABLE_OFFSET (seL4_WordBits - DIRECTORY_SIZE_BITS - TABLE_SIZE_BITS)
#define CAP_OFFSET (seL4_WordBits - CAPS_INDEX_BITS)

#define DIRECTORY_MASK (MASK(DIRECTORY_SIZE_BITS) << DIRECTORY_OFFSET)
#define TABLE_MASK (MASK(TABLE_SIZE_BITS) << TABLE_OFFSET)
#define CAP_MASK (MASK(CAPS_INDEX_BITS) << CAP_OFFSET)

#define CAP_INDEX(x) ((x & CAP_MASK) >> CAP_OFFSET)
#define DIRECTORY_INDEX(x) ((x & DIRECTORY_MASK) >> DIRECTORY_OFFSET)
#define TABLE_INDEX(x) ((x & TABLE_MASK) >> TABLE_OFFSET)

/* Struct for the top level of the page table */
typedef struct {
    seL4_Word *directory; /* Virtual address to the top level page directory */
    seL4_CPtr *kernel_page_table_caps; /* Virtual address to an array of in-kernel page table caps */
} page_directory;

/* WARNING: If this grows in size, algorithms will have to change */
typedef struct {
	seL4_CPtr page;
} page_table_entry;

/* 
 * Handle a vm fault.
 */ 
void vm_fault(void);

/*
 * Create a page directory
 * @returns page_directory object
 */
page_directory *page_directory_create(void);

/*
 * Insert a page into the two level page table
 * @param directory, the page directory to insert into
 * @param vaddr, the virtual address of the page
 * @param sos_cap, the capability of the page created by sos
 * @param kernel_cap, a capability of the page table created by the kernel
 * @returns 0 on success else 1
 */
int page_directory_insert(page_directory *directory, seL4_Word vaddr, seL4_CPtr sos_cap, seL4_CPtr kernel_cap);


/*
 * Given a vaddr, retrieve the cap for the page
 * @param directory, the page directory to insert into
 * @param vaddr, the virtual address of the page
 * @param cap, the cap for the page represented by vaddr
 * @returns 0 on success else 1
 */
int page_directory_lookup(page_directory *directory, seL4_Word vaddr, seL4_CPtr *cap);

/*
 * Given a vaddr, try to map in that page 
 * @param as, the address space to map into
 * @param vaddr, the vaddr of the page to map in
 * @param kvaddr[out], the kvaddr of the frame
 * @returns 0 on success else 1
 */
int vm_try_map(seL4_Word vaddr, seL4_Word access_type, seL4_Word *kvaddr);

#endif /* _VM_H_ */
