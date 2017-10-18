/*
 * Virtual File System
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include "vfs.h"

#include "device.h"
#include <fcntl.h>
#include <stdlib.h>
#include <utils/util.h>

static int vfs_lookup(char *name, int create_file, vnode **ret);

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
        LOG_ERROR("Failed to create device mount");
        return 1;
    }

    device_mount->vn_data = NULL;
    device_mount->vn_ops = &device_vnode_ops;
    if (vfs_mount(device_mount) != 0) {
        LOG_ERROR("Failed to mount device namespace");
        return 1;
    }

    return 0;
}

int
vfs_mount(vnode *vn)
{
    /*
     * Reach the end of the linked list
     * We want to do this in order to preserve the lookup order,
     * It's first come first serve for namespaces.
     */
    mount *curr;
    for (curr = mount_points; curr != NULL && curr->next != NULL; curr = curr->next);

    mount *new_mount = malloc(sizeof(mount));
    if (new_mount == NULL) {
        LOG_ERROR("Failed to create mount point entry");
        return 1;
    }

    new_mount->node = vn;
    new_mount->next = NULL;

    /* Either at the front of the linked list or not */
    if (curr == NULL) {
        mount_points = new_mount;
    } else {
        curr->next = new_mount;
    }

    return 0;
}

int
vfs_open(char *name, fmode_t mode, vnode **ret)
{
    vnode *vn = NULL;

    if (mode != O_RDONLY && mode != O_WRONLY && mode != O_RDWR) {
        LOG_ERROR("Invalid mode %d", mode);
        return 1;
    }

    /*
     * Might create a file, NFS creates a failed lookup file inside its vop_lookup function.
     * Only create the file if it is opened in writeable mode.
     */
    int create_file = 0;
    if (mode == O_WRONLY || mode == O_RDWR)
        create_file = 1;

    if (vfs_lookup(name, create_file, &vn) != 0) {
        LOG_ERROR("Failed to find the file");
        return 1;
    }

    /* Open the vnode */
    if (vn->vn_ops->vop_open(vn, mode) != 0) {
        LOG_ERROR("Failed to open the file");
        return 1;
    }

    *ret = vn;
    return 0;
}

void
vfs_close(vnode *vn, fmode_t mode)
{
    /* Delegate to the close operation */
    vn->vn_ops->vop_close(vn, mode);
}

int
vfs_stat(char *name, sos_stat_t **buf)
{
    vnode *vn = NULL;

    if (vfs_lookup(name, 0, &vn) != 0) {
        LOG_ERROR("Failed to find the file");
        return 1;
    }

    if (vn->vn_ops->vop_stat(vn, buf) != 0) {
        LOG_ERROR("Failed to stat the file");
        return 1;
    }

    return 0;
}

/*
 * Lookup a name inside the VFS
 * @param name, the name to search
 * @param create_file, flag to specify if a file should be created on lookup fail
 * This is delegated to the FS to first see this file and support file creation
 * @param[out] ret, the returned vnode
 * @returns 0 on success else 1
 */
static int
vfs_lookup(char *name, int create_file, vnode **ret)
{
    /*
     * Lookup name in each mount point, ordered by earliest to latest mount
     * The first namespace to recognise the file will be chosen
     */
    for (mount *curr = mount_points; curr != NULL; curr = curr->next) {
        if (curr->node->vn_ops->vop_lookup(name, create_file, ret) == 0)
            return 0;
    }

    return 1; /* Lookup failed */
}

int
vfs_list(char ***dir, size_t *nfiles)
{
    /* Create a master list which stores all entries */
    *nfiles = 0;
    char **master_list = malloc(sizeof(char *));
    if (master_list == NULL) {
        LOG_ERROR("Failed to create master list");
        return 1;
    }

    /* The namespace list stores netries from that namespace */
    char **namespace_list;
    size_t num_files = 0;

    for (mount *curr = mount_points; curr != NULL; curr = curr->next) {
        if (curr->node->vn_ops->vop_list(&namespace_list, &num_files) != 0) {
            LOG_ERROR("Failed to list files in namespace");
            return 1;
        }

        /* Add entries from namespace_list to the master list */
        size_t new_size = (*nfiles) + num_files;
        master_list = realloc(master_list, sizeof(char *) * new_size);
        for (int i = 0; i < num_files; i++)
            master_list[*nfiles + i] = namespace_list[i];

        *nfiles = new_size;
        free(namespace_list);
    }

    *dir = master_list;
    return 0;
}

vnode *
vnode_create(void *data, const void *ops, seL4_Word readcount, seL4_Word writecount)
{
    vnode *node = malloc(sizeof(vnode));
    if (node == NULL) {
        LOG_ERROR("Failed to malloc memory for a vnode");
        return NULL;
    }

    node->vn_data = data;
    node->vn_ops = ops;
    node->readcount = readcount;
    node->writecount = writecount;

    return node;
}
