/*
 * Virtual File System
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#ifndef _VFS_H_
#define _VFS_H_

#include <sel4/sel4.h>

#include <sos.h>


struct vnode;

typedef struct {
    // int (*vop_open)(vnode *object, int flags_from_open);
    // int (*vop_close)(vnode *vnode);
    int (*vop_read)(struct vnode *node, struct iovec *iov);
    int (*vop_write)(struct vnode *node, struct iovec *iov);

    int (*vop_lookup)(char *pathname, struct vnode **result); /* Lookup for a mount point */
} vnode_ops;

typedef struct {
    void *vn_data; /* Implementation specific data */
    const vnode_ops *vn_ops; /* Operations on a vnode */
} vnode;

/*
 * Initialise the VFS
 * @returns 0 on success else 1
 */
int vfs_init(void);

/*
 * Mount a vnode onto the VFS.
 * Represents a namespace that is now added into the lookup space.
 * @param vn, the vnode with vop_lookup defined
 * @returns 0 on success else 1
 */
int vfs_mount(vnode *vn);

/*
 * Open a 'file' in the VFS.
 * Finds in the vnode and calls vop_open.
 * @param path, the path of the file
 * @param mode, the mode of access
 * @param[out] ret, the returned vnode
 * 
 * @returns 0 on success else 1
 */
int vfs_open(char *path, fmode_t mode, vnode **ret);

/*
 * Close a 'file' in the VFS
 * calls vop_close on the vnode
 * @param vn, the node
 */
void vfs_close(vnode *vn); // TODO, RETURN ERROR?

/*
 * Lookup a path name inside the VFS
 * @param path, the name to search
 * @param[out] ret, the returned vnode
 * @returns 0 on success else 1
 */
int vfs_lookup(char *path, vnode **ret);

#endif /* _VFS_H_ */
