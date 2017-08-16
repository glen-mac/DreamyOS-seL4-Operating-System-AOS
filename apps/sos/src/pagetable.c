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


page_directory_t *
page_directory_create(void)
{
    page_directory_t *directory = malloc(BIT(DIRECTORY_SIZE_BITS) * sizeof(page_table_t));
    return directory;
}

int 
page_directory_insert(page_directory_t *table, void *data)
{
    return 0;
}
