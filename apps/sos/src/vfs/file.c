/*  
 * Implementations for file handle and file table management.
 */

#include "file.h"

#include <assert.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/panic.h>

#include <utils/util.h>

/*
 * The global open file table (OFT).
 *
 * The global OFT contains FILES_MAX file structures.
 * The default size is 8192 maximum open files in the system
 */

static ssize_t fdtable_next_unused_index(file **table);

int
file_open(char *filename, fmode_t mode, file **open_file)
{
    int result;

    /* Since we dont have an open file table, we can just keep creating more files */
    *open_file = malloc(sizeof(file));
    if (!open_file)
        return 1;

    vnode *node;
    if ((result = vfs_open(filename, mode, &node)) != 0) {
        free(*open_file);
        return result;
    }

    /* Set up the file entry. */
    (*open_file)->vn = node;
    (*open_file)->fp = 0;
    (*open_file)->mode = mode;

    return 0;
}

void
file_close(file *f)
{
    vfs_close(f->vn);
    f->vn = NULL;
}

fdtable *
fdtable_create(void)
{
    fdtable *fdt;

    if ((fdt = malloc(sizeof(fdtable))) == NULL)
        return NULL;

    // TODO this is needed why??.
    for (int i = 0; i < PROCESS_MAX_FILES; i++)
        fdt->table[i] = NULL;

    return fdt;
}


int
fdtable_get(fdtable *fdt, int fd, file **f)
{
    if (fd < 0 || (size_t)fd >= PROCESS_MAX_FILES)
        return EBADF;

    file *fdt_file = fdt->table[fd];
    if (fdt_file == NULL)
        return EBADF;

    *f = fdt_file;

    return 0;
}


void
fdtable_insert(fdtable *fdt, int fd, file *open_file)
{
    assert(fd >= 0 && open_file != NULL);
    assert(fdt->table[fd] == NULL);

    fdt->table[fd] = open_file;
}
 
int
fdtable_get_unused_fd(fdtable *fdt, int *fd)
{
    if ((*fd = fdtable_next_unused_index(fdt->table)) == -1)
        return EMFILE; /* Process out of space for new file */

    return 0;
}


int
fdtable_close_fd(fdtable *fdt, int fd, file **oft_file)
{
    if (fd < 0 || (unsigned)fd >= PROCESS_MAX_FILES)
        return EBADF;

    if ((*oft_file = fdt->table[fd]) == NULL)
        return EBADF;

    fdt->table[fd] = NULL;

    return 0;
}

/*
 * Finds the next unused (i.e., NULL) index in an array, and returns
 * it.  Unfortunately, this is a linear scan.
 *
 * @param table the array to search in
 * @returns the next unused index, or -1 if none was found.
 */
static ssize_t
fdtable_next_unused_index(file **table)
{
    for (size_t i = 0; i < PROCESS_MAX_FILES; i++) {
        LOG_INFO("Table[%i] is %p", i, table[i]);
        if (table[i] == NULL) 
            return i;
    }

    /* No slots free. */
    return -1;
}
