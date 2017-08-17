/*
 * Page table interface
 *
 */

#include <sel4/sel4.h>
#include "pagetable.h"

#include <assert.h>
#include <strings.h>

#include <cspace/cspace.h>
#include <utils/page.h>
#include <utils/util.h>

#include "frametable.h"

#define DIRECTORY_OFFSET (32 - DIRECTORY_SIZE_BITS)
#define TABLE_OFFSET (32 - DIRECTORY_SIZE_BITS - TABLE_SIZE_BITS)

#define DIRECTORY_MASK (MASK(DIRECTORY_SIZE_BITS) << DIRECTORY_OFFSET)
#define TABLE_MASK (MASK(TABLE_SIZE_BITS) << TABLE_OFFSET)

#define DIRECTORY_INDEX(x) ((x & DIRECTORY_MASK) >> DIRECTORY_OFFSET)
#define TABLE_INDEX(x) ((x & TABLE_MASK) >> TABLE_OFFSET)

page_directory_t *
page_directory_create(void)
{   
    seL4_Word directory_vaddr;
    seL4_Word kernel_cap_table_vaddr;
    if (frame_alloc(&directory_vaddr) == -1)
        return (page_directory_t *)NULL;

    if (frame_alloc(&kernel_cap_table_vaddr) == -1)
        return (page_directory_t *)NULL;

    page_directory_t *top_level = malloc(sizeof(page_directory_t));
    if (!top_level)
        return (page_directory_t *)NULL;

    top_level->directory = (seL4_Word *)directory_vaddr;
    top_level->kernel_cap_table = (seL4_Word *)kernel_cap_table_vaddr;

    return top_level;
}

int 
page_directory_insert(page_directory_t *table, seL4_Word vaddr, seL4_CPtr cap)
{
    seL4_Word directory_index = DIRECTORY_INDEX(vaddr);
    seL4_Word table_index = TABLE_INDEX(vaddr);

    /* Error if table doesnt exist for some reason */
    if (!table || !(table->directory)) {
        LOG_ERROR("Directory doesnt exist");
        return 1;
    }

    seL4_Word *directory = table->directory;
   /* LOG_INFO("directory %p", directory);
    LOG_INFO("directory index %u", directory_index);
    LOG_INFO("table index %u", table_index);
    */
    /* Alloc the second level if it doesnt exist */
    if (!directory[directory_index]) {
        LOG_INFO("Creating second level page table at index %d", directory_index);
        seL4_Word page_table_vaddr;
        if (frame_alloc(&page_table_vaddr) == -1)
            return 1;

        directory[directory_index] = (seL4_Word *)page_table_vaddr;
    }

    seL4_Word *second_level = directory[directory_index];
    assert(second_level[table_index] == (seL4_CPtr)NULL); 
    second_level[table_index] = cap;

    return 0;
}
