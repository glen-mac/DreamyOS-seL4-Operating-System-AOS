/*
 * Wrappers for the NFS file system
 * 
 * Glenn McGuire & Cameron Lonsdale
 */

#ifndef _SOS_NFS_H_
#define _SOS_NFS_H_

#include <vfs/vfs.h>
#include <sos.h>

/*
 * Initialise the NFS file system
 * @returns 0 on success, else 1
 */
int sos_nfs_init(void);

/*
 * NFS lookup from the namespace
 * @param name, the name of the file
 * @param create_file, flag to specify if a file is to be created
 * @param[out] result, the returned vnode
 * @returns 0 on success, else 1
 */
int sos_nfs_lookup(char *name, int create_file, vnode **result);

/*
 * List all files
 * @param[out] list, the list of file names
 * @param[out] nfiles, the number of files found
 * @returns 0 on success, else 1
 */
int sos_nfs_list(char ***list, size_t *nfiles);

/*
 * Open an NFS file
 * @param vnode, vnode of the file
 * @param mode, mode of access
 * @returns 0 on success, else 1
 */
int sos_nfs_open(vnode *vnode, fmode_t mode);

/*
 * Write to an NFS file
 * @param node, the vnode of the file
 * @param iov, the io vector
 * @returns nbytes written on success else -1
 */
int sos_nfs_write(vnode *node, uiovec *iov);

/*
 * Read from an NFS file
 * @param node, the vnode of the file
 * @param iov, the io vector
 * @returns nbytes read on success else -1
 */
int sos_nfs_read(vnode *node, uiovec *iov);

/*
 * Get attributes of an NFS file
 * @param node, the vnode of the file
 * @param stat, the stat struct
 * @returns 0 on success, else 1
 */
int sos_nfs_stat(vnode *node, sos_stat_t **stat);

/*
 * Close an NFS file
 * @param node, the vnode of the file
 * @param mode, the mode of access held
 * @returns 0 on success else 1
 */
int sos_nfs_close(vnode *node, fmode_t mode);

#endif /*_SOS_NFS_H_ */
