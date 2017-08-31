/*
 * Wrappers for the NFS file system
 * 
 * Glenn McGuire & Cameron Lonsdale
 */

#ifndef _SOS_NFS_H_
#define _SOS_NFS_H_

#include <vfs/vfs.h>
#include <sys/uio.h>

/*
 * Initialise the NFS file system
 * @returns 0 on success else 1
 */
int sos_nfs_init(void);

/*
 * NFS lookup from the namespace
 * @param name, the name of the file
 * @param create_file, flag to specify if a file is to be created
 * @param[out] ret, the returned vnode
 * @returns 0 on success, else 1
 */
int sos_nfs_lookup(char *name, int create_file, vnode **result);

/*
 * List all files
 * @param[out] list, the list of names
 * @param[out] nfiles, the number of files
 * @returns 0 on success, else 1
 */
int sos_nfs_list(char ***list, size_t *nfiles);

/*
 * List all devices
 * @param[out] list, the list of names
 * @param[out] nfiles, the number of files listed
 * @returns 0 on success, else 1
 */
int device_list(char ***list, size_t *nfiles);

/*
 * Write to an NFS file
 * @param node, the vnode of the file
 * @param iov, the io vector
 * @returns nbytes written on success else errno
 */
int sos_nfs_write(vnode *node, uiovec *iov);

/*
 * Read from an NFS file
 * @param node, the vnode of the file
 * @param iov, the io vector
 * @returns nbytes read on success else errno
 */
int sos_nfs_read(vnode *node, uiovec *iov);

/*
 * Close an NFS file
 * @param node, the vnode of the file
 * @returns 0 on success else -1
 */
int sos_nfs_close(vnode *node);

#endif /*_SOS_NFS_H_ */
