/*
 * Device Management
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include "device.h"

#include "vfs.h"

#include <utils/util.h>
#include <string.h>

#include <stdlib.h>

/*
 * Device representation
 * Has a name and a vnode attached
 */
typedef struct dev {
    char *name;
    vnode *vn;
    struct dev *next;
} device;


const vnode_ops device_vnode_ops = {
    .vop_lookup = device_lookup
};

/*
 * Linked List of registered devices.
 * O(n) lookup, not ideal
 */
static vnode *devices = NULL;

int
device_register(char *name, vnode *vn)
{
    device *new_dev = malloc(sizeof(device));
    if (!new_dev)
        return 1;

    new_dev->name = name;
    new_dev->vn = vn;

    device *head = devices;
    new_dev->next = head;
    devices = new_dev;

    return 0;
}

int
device_lookup(char *name, vnode **ret)
{
    LOG_INFO("help");

    for (device *curr = devices; curr != NULL; curr = curr->next) {
        LOG_INFO("looking at %s", name);
        LOG_INFO("curr dev %p", curr);
        LOG_INFO("Curr->name, %s", curr->name);
        /* Try Lookup the file in this namespace */
        if (!strcmp(name, curr->name)) {
            *ret = curr->vn;
            return 0;
        }
    }

    return 1; /* Lookup failed */
}
