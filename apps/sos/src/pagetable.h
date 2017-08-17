/*
 * Page table interface
 *
 */

#ifndef _PAGETABLE_H_
#define _PAGETABLE_H_

#include <sel4/sel4.h>

#define DIRECTORY_SIZE_BITS 10
#define TABLE_SIZE_BITS 10

/* Struct for the top level of the page table */
typedef struct {
    seL4_Word *directory; /* virtual address to the top level page directory */
    seL4_Word *kernel_cap_table; /* virtual address to an array of in-kernel page table caps */
} page_directory_t;

/* Struct for the second level of the page table */
typedef struct {
    seL4_CPtr cap;
    seL4_CPtr vaddr;
} page_table_t;

page_directory_t *page_directory_create(void);
int page_directory_insert(page_directory_t *table, seL4_Word vaddr, seL4_CPtr cap);

#endif /* _PAGETABLE_H_ */