/*
 * Page table interface
 *
 */

#ifndef _PAGETABLE_H_
#define _PAGETABLE_H_

#include <sel4/sel4.h>

#define DIRECTORY_SIZE_BITS 12
#define TABLE_SIZE_BITS 8

/* Struct for the top level of the page table */
typedef struct {
    seL4_CPtr cap;
    seL4_CPtr vaddr;
} page_directory_t;

/* Struct for the second level of the page table */
typedef struct {
    seL4_CPtr cap;
    seL4_CPtr vaddr;
} page_table_t;

page_directory_t *page_directory_create(void);
int page_directory_insert(page_directory_t *table, void *data);

#endif /* _PAGETABLE_H_ */