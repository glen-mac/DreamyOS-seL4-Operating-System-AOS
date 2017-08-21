/*
 * Declarations for file handle and file table management.
 */

#ifndef _FILE_H_
#define _FILE_H_

#include <sel4/sel4.h>
#include <sel4/types.h>

#include <sos.h>

#include "vfs.h"

/* File structure stored in global open file table */
typedef struct {
    off_t fp;
    vnode *vn;
    int mode;
    size_t refcnt;
} file;

/*
 * The per-process file descriptor table (FDT).
 */
typedef struct {
    seL4_Word *table[PROCESS_MAX_FILES];
} fdtable;


/*
 * Open a file specified by filename, flags and mode using VFS.
 * Setup file metadata for use in the global open file table.
 *
 * @returns 0 on success, or an errno.
 * @param filename, name of the file
 * @oaram mode, mode of opening
 * @param[out] oft_offset, the OFT entry
 */
int file_open(char *filename, fmode_t mode, file **oft_file);

/*
 * Close a file from the global OFT.
 * If there are no more references to this file being open,
 * the file is closed.
 *
 * @param f, the file to close
 * @returns the reference count for this file
 */
void file_close(file *f);

/*
 * Initialises a per-process file descriptor table.
 *
 * @returns a pointer to the new fdt, or NULL
 */
fdtable *fdtable_create(void);

/*
 * For a particular process, return the file corresponding to a file
 * descriptor number.
 *
 * @param fdt the per-process file descriptor table
 * @param fd the file descriptor number
 * @param[out] f the file to retrieve
 * @returns 0 on success or a failure value
 */
int fdtable_get(fdtable *table, int fd, file **f);

/*
 * Insert a file into the per-process file-descriptor table.
 *
 * @param fdt the per-process file descriptor table (pFDT)
 * @param fd the "file descriptor number", corresponding to an index in the pFDT.
 * @param oft_file the file from the global open file table (OFT)
 */
void fdtable_insert(fdtable *table, int fd, file *oft_file);

/*
 * Get an unused file descriptor from fdt
 *
 * @param fdt the per-process file descriptor table (pFDT)
 * @param[out] fd the "file descriptor number", corresponding to an index
 *     in the pFDT.)
 * @returns 0 on success and EMFILE on failure
 */
int fdtable_get_unused_fd(fdtable *table, int *fd);

/*
 * Close a fd from a fdt
 *
 * @param fdt the per-process file descriptor table (pFDT)
 * @param fd the "file descriptor number", corresponding to an index
 *     in the pFDT.)
 * @param[out] oft_file file which was referenced by fd
 * @returns 0 on success and EBADF on failure
 */
int fdtable_close_fd(fdtable *table, int fd, file **oft_file);

#endif /* _FILE_H_ */