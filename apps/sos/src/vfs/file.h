/*
 * Declarations for file handle and file table management.
 *
 * Cameron Lonsdale & Glenn McGuire
 */

#ifndef _FILE_H_
#define _FILE_H_

#include "vfs.h"

/* File structure */
typedef struct {
    off_t fp;  /* file pointer, current location in the file */
    vnode *vn; /* Vnode attached to this file */
    int mode;  /* Mode of access */
} file;

/*
 * The per-process file descriptor table (FDT).
 */
typedef struct {
    file *table[PROCESS_MAX_FILES];
} fdtable;

/*
 * Open a file given a filename, flags and mode, using the VFS.
 * @param filename, name of the file
 * @oaram mode, mode of opening
 * @param[out] open_file, the opened file
 * @returns 0 on success, else 1
 */
int file_open(char *filename, fmode_t mode, file **open_file);

/*
 * Close a file
 * Delegate to the VFS to close the file
 * @param f, the file to close
 */
void file_close(file *f);

/*
 * Initialises a per-process file descriptor table.
 * @returns a pointer to the new fdt, or NULL on error
 */
fdtable *fdtable_create(void);

/*
 * Destroys a fdtable
 * All files in the table are closed
 * @param table, the fdtable to destroy
 * @returns 0 on success else 1
 */
int fdtable_destroy(fdtable *table);

/*
 * Return the file corresponding to a file descriptor number.
 * @param table, the file descriptor table
 * @param fd,  the file descriptor number
 * @param[out] f,  the file to retrieve
 * @returns 0 on success, else 1
 */
int fdtable_get(fdtable *table, int fd, file **f);

/*
 * Insert a file into a file descriptor table.
 * @param table, the file descriptor table
 * @param fd, the file descriptor, corresponding to an index in the fdtable.
 * @param open_file, the file to insert
 */
void fdtable_insert(fdtable *table, int fd, file *open_file);

/*
 * Get an unused file descriptor from a fdtable
 * @param table, the file descriptor table
 * @param[out] fd, the unuused file descriptor
 * @returns 0 on success, else 1
 */
int fdtable_get_unused_fd(fdtable *table, int *fd);

/*
 * Close a fd
 * @param table, the file descriptor table
 * @param fd, the fd to close
 * @param[out] open_file, the open file referenced by fd
 * @returns 0 on success, else 1
 */
int fdtable_close_fd(fdtable *table, int fd, file **open_file);

#endif /* _FILE_H_ */
