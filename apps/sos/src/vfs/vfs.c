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
    if (!device_mount) {
        LOG_ERROR("Malloc failed when creating device namespace");
        return 1;
    }

    device_mount->vn_data = NULL;
    device_mount->vn_ops = &device_vnode_ops;
    if (vfs_mount(device_mount) != 0) {
        LOG_ERROR("Error mounting device namespace");
        return 1;
    }
   
    return 0;
}

int
vfs_mount(vnode *vn)
{
    mount *curr;
    for (curr = mount_points; curr != NULL && curr->next != NULL; curr = curr->next);

    mount *new_mount = malloc(sizeof(mount));
    if (!new_mount) {
        LOG_ERROR("malloc error creating mount point");
        return 1;
    }

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
vfs_open(char *name, fmode_t mode, vnode **ret)
{
    int result;
    vnode *vn = NULL;

    if (mode != O_RDONLY && mode != O_WRONLY && mode != O_RDWR) {
        LOG_ERROR("incorrect mode %d", mode);
        return EINVAL;
    }

    /*
     * Might create a file, NFS creates a failed lookup file inside its vop_lookup function
     * Only create the file if it is opened in write only mode aswell
     */
    int create_file = 0;
    if (mode == O_WRONLY || mode == O_RDWR)
        create_file = 1;

    if ((result = vfs_lookup(name, create_file, &vn))) {
        LOG_ERROR("vfs_lookup failed");
        return result;
    }

    // Not actually necessary now, lookup sort of opens it for us..?
    // We dont share vnodes and have refcount, so this might change? idk

    // TODO: I think we do need vop_open, because we need to update the access time and things for files

    // if (vn->vn_ops->vop_open(vn, mode) == 0)
    //     return 0;

    *ret = vn;
    return 0;
}

void
vfs_close(vnode *vn)
{
    /* Delegate to the close operation */
    vn->vn_ops->vop_close(vn);
}

int
vfs_stat(char *name, sos_stat_t **buf)
{
    int result;
    vnode *vn = NULL;

    if ((result = vfs_lookup(name, 0, &vn))) {
        LOG_ERROR("vfs_lookup failed");
        return result;
    }

    if (vn->vn_ops->vop_stat(vn, buf) != 0)
        return 1;

    return 0;
}

int
vfs_lookup(char *name, int create_file, vnode **ret)
{
    for (mount *curr = mount_points; curr != NULL; curr = curr->next) {
        if (curr->node->vn_ops->vop_lookup(name, create_file, ret) == 0)
            return 0;
    }

    return 1; /* Lookup failed */
}

int
vfs_list(char ***master_list, size_t *num_files)
{
    *num_files = 0;
    char **big_list = malloc(sizeof(char *));
    if (!big_list)
        return 1;

    char **dir;
    size_t nfiles = 0;

    for (mount *curr = mount_points; curr != NULL; curr = curr->next) {
        if (curr->node->vn_ops->vop_list(&dir, &nfiles) != 0)
            return 1;

        /* Resize list to fit new entries */
        size_t new_size = (*num_files) + nfiles;
        big_list = realloc(big_list, sizeof(char *) * new_size);
        for (int i = 0; i < nfiles; i++)
            big_list[*num_files + i] = dir[i];

        *num_files = new_size;
        free(dir);
    }

    *master_list = big_list;
    return 0;
}
