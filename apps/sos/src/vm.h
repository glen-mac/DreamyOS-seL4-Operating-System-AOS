/*
 * Virtual memory interface
 * Cameron Lonsdale & Glenn McGuire
 *
 */

#ifndef _VM_H_
#define _VM_H_

#include <sel4/sel4.h>

/* Struct for the top level of the page table */
typedef struct {
    seL4_Word *directory; /* Virtual address to the top level page directory */
    seL4_CPtr *kernel_page_table_caps; /* Virtual address to an array of in-kernel page table caps */
} page_directory;

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
 * @param table, the page directory to insert into
 * @param vaddr, the virtual address of the page
 * @param sos_cap, the capability of the page created by sos
 * @param kernel_cap, a capability of the page table created by the kernel
 */
int page_directory_insert(page_directory *directory, seL4_Word vaddr, seL4_CPtr sos_cap, seL4_CPtr kernel_cap);

#endif /* _VM_H_ */