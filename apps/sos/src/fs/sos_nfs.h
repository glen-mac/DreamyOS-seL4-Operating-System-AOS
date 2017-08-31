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
 * Get attributes of an NFS file
 * @param node, the vnode of the file
 * @param stat, the stat struct
 * @returns 0 on success, else 1
 */
int sos_nfs_getattr(vnode *node, sos_stat_t *stat);


/*
 * Close an NFS file
 * @param node, the vnode of the file
 * @returns 0 on success else -1
 */
int sos_nfs_close(vnode *node);

#endif /*_SOS_NFS_H_ */
