/*
 * Virtual File System
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#ifndef _VFS_H_
#define _VFS_H_

#include <sel4/sel4.h>
#include <sys/uio.h>

#include <sos.h>

typedef struct {
	void *uiov_base;
	seL4_Word uiov_len;
	off_t uiov_pos;
} uiovec;

typedef struct _vnode vnode;

typedef struct {
    int (*vop_open)(vnode *vnode, fmode_t mode);
    int (*vop_close)(vnode *vnode, fmode_t mode);
    int (*vop_read)(vnode *node, uiovec *iov);
    int (*vop_write)(vnode *node, uiovec *iov);
    int (*vop_stat)(vnode *node, sos_stat_t **buf);

    int (*vop_lookup)(char *name, int create_file, vnode **result); /* Lookup for a mount point */
    int (*vop_list)(char ***dir, size_t *nfiles); /* list all the files in a mount point */
} vnode_ops;

typedef struct _vnode {
    void *vn_data; /* Implementation specific data */
    const vnode_ops *vn_ops; /* Operations on a vnode */
    seL4_Word readcount; /* Number of read references on this node */
    seL4_Word writecount; /* Number of read references on this node */
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
 * @param name, the name of the file
 * @param mode, the mode of access
 * @param[out] ret, the returned vnode
 * @returns 0 on success else 1
 */
int vfs_open(char *name, fmode_t mode, vnode **ret);

/*
 * Close a 'file' in the VFS
 * calls vop_close on the vnode
 * @param vn, the node
 * @param mode, the mode the vnode was opened with
 */
void vfs_close(vnode *vn, fmode_t mode);

/*
 * Get the attributes of a file
 * @param vn, the node
 * @param buf, buffer to store file attributes
 * @returns 0 on success, else 1
 */
int vfs_stat(char *name, sos_stat_t **buf);

/*
 * Lookup a name inside the VFS
 * @param name, the name to search
 * @param create_file, flag to specify if a file should be created on lookup fail
 * This is delegated to the FS to first see this file and support file creation
 *
 * @param[out] ret, the returned vnode
 * @returns 0 on success else 1
 */
int vfs_lookup(char *name, int create_file, vnode **ret);

/*
 * List all the files in the VFS
 */
int vfs_list(char ***dir, size_t *nfiles);

#endif /* _VFS_H_ */
