/*
 * Wrappers for the NFS file system
 * 
 * Glenn McGuire & Cameron Lonsdale
 */

#ifndef _SOS_NFS_H_
#define _SOS_NFS_H_

#include <vfs/vfs.h>
#include <sys/uio.h>

int sos_nfs_init(void);

/*
 * NFS lookup from the namespace
 * @param name, the name of the file
 * @param[out] ret, the returned vnode
 * @returns 0 on success, else 1
 */
int sos_nfs_lookup(char *name, vnode **result);

/*
 * Write to an NFS file
 * @param node, the vnode of the file
 * @param iov, the io vector
 * @returns nbytes written on success else errno
 */
int sos_nfs_write(vnode *node, struct iovec *iov);

/*
 * Read from an NFS file
 * @param node, the vnode of the file
 * @param iov, the io vector
 * @returns nbytes read on success else errno
 */
int sos_nfs_read(vnode *node, struct iovec *iov);

#endif /*_SOS_NFS_H_ */
