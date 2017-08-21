/*  
 * Implementations for file handle and file table management.
 */

#include "file.h"

#include <assert.h>
#include <stdlib.h>
#include <errno.h>

#define FILES_MAX 8192

/*
 * The global open file table (OFT).
 *
 * The global OFT contains FILES_MAX file structures.
 * The default size is 8192 maximum open files in the system
 */
static file oft[FILES_MAX];

static ssize_t fdtable_next_unused_index(seL4_Word *table);
static ssize_t get_empty_filp(file **f);

int
file_open(char *filename, fmode_t mode, file **oft_file)
{
    int result;

    /* 
     * set file to point to a file structure that has a null vnode 
     * return the file in a locked state
     */
    if (get_empty_filp(oft_file) == -1)
        /* System out of space for new file. */
        return ENFILE;

    vnode *node;
    if ((result = vfs_open(filename, mode, &node)) != 0)
        return result;

    /* Set up the file entry. */
    (*oft_file)->vn = node;
    (*oft_file)->fp = 0;
    (*oft_file)->mode = mode;
    (*oft_file)->refcnt = 1;

    return 0;
}

void
file_close(file *f)
{
    if (--f->refcnt == 0) {
        vfs_close(f->vn);
        f->vn = NULL;
    }
}

fdtable *
fdtable_create(void)
{
    fdtable *fdt;

    if ((fdt = malloc(sizeof(fdtable))) == NULL)
        return NULL;

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
fdtable_insert(fdtable *fdt, int fd, file *oft_file)
{
    assert(fd >= 0 && oft_file != NULL);
    assert(fdt->table[fd] == NULL);

    fdt->table[fd] = oft_file;
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
fdtable_next_unused_index(seL4_Word *table)
{
    for (size_t i = 0; i < PROCESS_MAX_FILES; i++) {
        if (table[i] == NULL)
            return i;
    }

    /* No slots free. */
    return -1;
}

/*
 * Find an empty file.
 * Linear scan through all files in the oft
 *
 * @param[out] file structure that is unused
 * @returns 0 on success, -1 on failure
 */
static ssize_t
get_empty_filp(file **f)
{
    for (unsigned i = 0; i < FILES_MAX; ++i) {
        *f = &oft[i];
        if ((*f)->vn == NULL && (*f)->refcnt == 0)
            return 0;
    }
    return -1;
}