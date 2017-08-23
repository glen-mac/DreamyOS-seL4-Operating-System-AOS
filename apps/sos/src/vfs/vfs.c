/*
 * Virtual File System
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include "vfs.h"

#include "device.h"

#include <stdlib.h>
#include <assert.h>
#include <errno.h>

#include <utils/util.h>

#include <fcntl.h>

/*
 * Linked list of mount points on the VFS
 * Each mount point is a VFS with vop_lookup defined.
 */
typedef struct mnt {
    vnode *node;
    struct mnt *next;
} mount;

mount *mount_points = NULL;

int
vfs_init(void)
{
    /*
     * Mount the device name space to the VFS
     */
    vnode *device_mount = malloc(sizeof(vnode));
    if (!device_mount)
        return 1;

    device_mount->vn_data = NULL;
    device_mount->vn_ops = &device_vnode_ops;
    vfs_mount(device_mount);

    // TODO: mount nfs

    return 0;
}

int
vfs_mount(vnode *vn)
{
    mount *curr;
    for (curr = mount_points; curr != NULL && curr->next != NULL; curr = curr->next);

    mount *new_mount = malloc(sizeof(mount));
    if (!new_mount)
        return 1;

    new_mount->node = vn;
    new_mount->next = NULL;

    if (!curr) {
        mount_points = new_mount;
    } else {
        curr->next = new_mount;
    }

    return 0;
}

int
vfs_open(char *path, fmode_t mode, vnode **ret)
{
    int result;
    vnode *vn = NULL;

    if (mode != O_RDONLY && mode != O_WRONLY && mode != O_RDWR) {
        LOG_ERROR("what, %d", mode);
        return EINVAL;
    }

    // Might have to deal with file creation here for NFS
    if ((result = vfs_lookup(path, &vn)))
        return result;

    *ret = vn;

    // Not actually necessary now, lookup sort of opens it for us..?
    // We dont share vnodes and have refcount, so this might change? idk

    // if (vn->vn_ops->vop_open(vn, mode) == 0)
    //     return 0;

    return 0;
}

void
vfs_close(vnode *vn)
{
    assert(!"Not implemented");
    return 1;
}

int
vfs_lookup(char *path, vnode **ret)
{
    LOG_INFO("looking up %s", path);

    for (mount *curr = mount_points; curr != NULL; curr = curr->next) {
        LOG_INFO("curr is %p");
        LOG_INFO("curr->node is %p", curr->node);
        LOG_INFO("curr->node->vn_ops is %p", curr->node->vn_ops);
        LOG_INFO("curr->node->vn_ops->vop_lookup is %p", curr->node->vn_ops->vop_lookup);
        /* Try Lookup the file in this namespace */

        if (curr->node->vn_ops->vop_lookup(path, ret) == 0)
            return 0;
    }

    return 1; /* Lookup failed */
}
