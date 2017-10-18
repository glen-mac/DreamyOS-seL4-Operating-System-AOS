/*
 * Device Management
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include "device.h"

#include <string.h>
#include <stdlib.h>
#include <utils/util.h>
#include "vfs.h"

/*
 * Device representation
 * Linked list element, has a name and a vnode attached
 */
typedef struct dev {
    char *name;
    vnode *vn;
    struct dev *next;
} device;

/* Operations on the device namespace */
const vnode_ops device_vnode_ops = {
    .vop_lookup = device_lookup,
    .vop_list = device_list
};

/*
 * Linked List of registered devices.
 * O(n) lookup, not ideal
 */
static device *devices = NULL;

int
device_register(char *name, vnode *vn)
{
    device *new_dev = malloc(sizeof(device));
    if (new_dev == NULL) {
        LOG_ERROR("Failed to create new device");
        return 1;
    }

    /* Set name and vnode */
    new_dev->name = name;
    new_dev->vn = vn;

    /* Add to the head of the devices linked list */
    device *head = devices;
    new_dev->next = head;
    devices = new_dev;

    return 0;
}

int
device_lookup(char *name, int create_file, vnode **ret)
{
    for (device *curr = devices; curr != NULL; curr = curr->next) {
        /* Try Lookup the file in this namespace */
        if (!strcmp(name, curr->name)) {
            *ret = curr->vn;
            return 0;
        }
    }

    LOG_INFO("Lookup for %s failed", name);
    return 1;
}

int
device_list(char ***list, size_t *nfiles)
{
    /* Count the number of devices */
    for (device *curr = devices; curr != NULL; curr = curr->next)
        (*nfiles)++;

    char **dir = malloc(sizeof(char *) * (*nfiles));
    if (!dir) {
        LOG_ERROR("Failed to allocate memory for a device list");
        return 1;
    }

    /* Copy names into the directory */
    int i = 0;
    for (device *curr = devices; curr != NULL; curr = curr->next)
        dir[i++] = devices->name;

    *list = dir;
    return 0;
}
