/*  
 * Implementations for file handle and file table management.
 *
 * Cameron Lonsdale & Glenn McGuire
 */
#include "file.h"

#include <assert.h>
#include <strings.h>
#include <stdlib.h>
#include <errno.h>
#include <utils/util.h>

static ssize_t fdtable_next_unused_index(file **table);

int
file_open(char *filename, fmode_t mode, file **open_file)
{
    vnode *node;
    if (vfs_open(filename, mode, &node) != 0) {
        LOG_ERROR("Failed to open the vnode");
        return 1;
    }

    /* Since we dont have an open file table, we can just keep creating more files */
    if ((*open_file = malloc(sizeof(file))) == NULL) {
        LOG_ERROR("Failed to create a file");
        return 1;
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
    vfs_close(f->vn, f->mode);
    f->vn = NULL;
    free(f);
}

fdtable *
fdtable_create(void)
{
    fdtable *fdt;
    if ((fdt = malloc(sizeof(fdtable))) == NULL) {
        LOG_ERROR("Failed to create the fdtable");
        return NULL;
    }

    bzero(fdt->table, sizeof(file *) * PROCESS_MAX_FILES);
    return fdt;
}

int
fdtable_destroy(fdtable *table)
{
    file *open_file = NULL;
    for (size_t fd = 0; fd < PROCESS_MAX_FILES; ++fd) {
        /* If the fd is valid, close the file attached */
        if (fdtable_close_fd(table, fd, &open_file) == 0)
            file_close(open_file);
    }

    free(table);
    return 0;
}

int
fdtable_get(fdtable *fdt, int fd, file **f)
{
    if (!ISINRANGE(0, fd, PROCESS_MAX_FILES)) {
        LOG_ERROR("Invalid fd %d", fd);
        return 1;
    }

    if ((*f = fdt->table[fd]) == NULL) {
        LOG_ERROR("%d didnt correspond to an open file", fd);
        return 1;
    }

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
    if ((*fd = fdtable_next_unused_index(fdt->table)) == -1) {
        LOG_ERROR("Failed to find a free index in the fdtable");
        return 1; /* Process out of space for new file */
    }

    return 0;
}

int
fdtable_close_fd(fdtable *fdt, int fd, file **open_file)
{
    if (!ISINRANGE(0, fd, PROCESS_MAX_FILES)) {
        LOG_ERROR("Invalid fd %d", fd);
        return 1;
    }

    if ((*open_file = fdt->table[fd]) == NULL) {
        LOG_ERROR("Invalid file for fd %d", fd);
        return 1;
    }

    fdt->table[fd] = NULL;
    return 0;
}

/*
 * Finds the next unused (i.e., NULL) index in an array, and returns
 * it.  Unfortunately, this is a linear scan.
 * @param table,  the array to search in
 * @returns the next unused index, or -1 if none was found.
 */
static ssize_t
fdtable_next_unused_index(file **table)
{
    for (size_t i = 0; i < PROCESS_MAX_FILES; i++) {
        if (table[i] == NULL) 
            return i;
    }

    /* No slots free. */
    return -1;
}
