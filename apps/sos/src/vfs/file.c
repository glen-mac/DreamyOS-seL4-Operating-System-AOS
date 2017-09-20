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
    int result;

    /* Since we dont have an open file table, we can just keep creating more files */
    *open_file = malloc(sizeof(file));
    if (!open_file) {
        LOG_ERROR("malloc error when creating a file");
        return 1;
    }

    vnode *node;
    if ((result = vfs_open(filename, mode, &node)) != 0) {
        free(*open_file);
        LOG_ERROR("vfs open failed");
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
    free(f);
}

fdtable *
fdtable_create(void)
{
    fdtable *fdt;
    if ((fdt = malloc(sizeof(fdtable))) == NULL) {
        LOG_ERROR("fdt table malloc failed");
        return NULL;
    }

    bzero(fdt->table, PROCESS_MAX_FILES);
    return fdt;
}

int
fdtable_destroy(fdtable *table)
{
    file *open_file;
    for (size_t fd = 0; fd < PROCESS_MAX_FILES; ++fd) {
        if (fdtable_close_fd(table, fd, &open_file) == 0)
            file_close(open_file);
    }

    free(table);
    return 0;
}


int
fdtable_get(fdtable *fdt, int fd, file **f)
{
    if (fd < 0 || (size_t)fd >= PROCESS_MAX_FILES) {
        LOG_ERROR("invalid fd %d", fd);
        return EBADF;
    }

    if ((*f = fdt->table[fd]) == NULL) {
        LOG_ERROR("%d didnt correspond to an open file", fd);
        return EBADF;
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
        LOG_ERROR("Proc out of space for an open file");
        return EMFILE; /* Process out of space for new file */
    }

    return 0;
}


int
fdtable_close_fd(fdtable *fdt, int fd, file **oft_file)
{
    if (fd < 0 || (unsigned)fd >= PROCESS_MAX_FILES) {
        LOG_ERROR("Invalid fd %d", fd);
        return EBADF;
    }

    if ((*oft_file = fdt->table[fd]) == NULL) {
        LOG_ERROR("Invalid file for fd %d", fd);
        return EBADF;
    }

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
        if (table[i] == NULL) 
            return i;
    }

    /* No slots free. */
    return -1;
}
