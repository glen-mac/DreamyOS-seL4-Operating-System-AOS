/*
 * Device Management
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#ifndef _DEVICE_H_
#define _DEVICE_H_

#include "vfs.h"

/*
 * Register a device under a name
 * @param name, the name of the device
 * @param vn, the node represeting the device
 * @returns 0 on success else 1
 */
int device_register(char *name, vnode *vn);

/*
 * Lookup a device
 * O(N) search through the linked list of known devices
 * @param name, the name of the device
 * @param[out] ret, the returned vnode
 * @returns 0 on success, else 1
 */
int device_lookup(char *name, vnode **ret);

extern const vnode_ops device_vnode_ops;

#endif /* _DEVICE_H_ */
