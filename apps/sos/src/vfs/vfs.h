/*
 * Virtual File System
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#ifndef _VFS_H_
#define _VFS_H_

#include <sel4/sel4.h>
#include <sos.h>

/* 
 * Input output vector
 * defines a base address, a length, and a position in the file
 */
typedef struct {
	void *uiov_base;
	seL4_Word uiov_len;
	off_t uiov_pos;
} uiovec;

/* Forward declaration for use in vnode_ops */
typedef struct _vnode vnode;

/* Operations on a vnode */
typedef struct {
    int (*vop_open)(vnode *vnode, fmode_t mode);
    int (*vop_close)(vnode *vnode, fmode_t mode);
    int (*vop_read)(vnode *node, uiovec *iov);
    int (*vop_write)(vnode *node, uiovec *iov);
    int (*vop_stat)(vnode *node, sos_stat_t **buf);

    int (*vop_lookup)(char *name, int create_file, vnode **result); /* Lookup for a mount point */
    int (*vop_list)(char ***dir, size_t *nfiles); /* list all the files in a mount point */
} vnode_ops;

/* Structure for a vndoe */
typedef struct _vnode {
    void *vn_data; /* Implementation specific data */
    const vnode_ops *vn_ops; /* Operations on a vnode */
    seL4_Word readcount; /* Number of read references on this node */
    seL4_Word writecount; /* Number of read references on this node */
} vnode;

/*
 * Initialise the VFS.
 * Mounts the device namespace.
 * @returns 0 on success else 1
 */
int vfs_init(void);

/*
 * Mount a vnode onto the VFS.
 * Represents a namespace that is now added into the lookup resolution path.
 * @param vn, the vnode with vop_lookup defined
 * @returns 0 on success else 1
 */
int vfs_mount(vnode *vn);

/*
 * Open a file in the VFS.
 * Finds in the vnode and calls vop_open.
 * May create file if opened with write permissions.
 * @param name, the name of the file
 * @param mode, the mode of access
 * @param[out] ret, the returned vnode
 * @returns 0 on success else 1
 */
int vfs_open(char *name, fmode_t mode, vnode **ret);

/*
 * Close a file in the VFS
 * Delegates to the nodes close operations
 * @param vn, the node
 * @param mode, the mode the vnode was opened with
 */
void vfs_close(vnode *vn, fmode_t mode);

/*
 * Get the attributes of a file
 * @param name, the name of the file to stat
 * @param buf, buffer to store file attributes
 * @returns 0 on success, else 1
 */
int vfs_stat(char *name, sos_stat_t **buf);

/*
 * List all the files in the VFS
 * @param[out] dir, an array of file names
 * @param[out] nfiles, the number of files in the directory
 * @returns 0 on success, else 1
 */
int vfs_list(char ***dir, size_t *nfiles);

/*
 * Create a vnode
 * @param data, the vn_data of the node
 * @param ops, the operations pointers for the node
 * @param readcount, the initial value for the number of current readers
 * @param writecount, the initial value for the number of current writers
 * @returns pointer to vnode on success, else NULL
 */
vnode *vnode_create(void *data, const void *ops, seL4_Word readcount, seL4_Word writecount);

#endif /* _VFS_H_ */
