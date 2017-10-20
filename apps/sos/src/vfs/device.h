/*
 * Device Management
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#ifndef _DEVICE_H_
#define _DEVICE_H_

#include "vfs.h"

/* Operations on the deviec namespace */
extern const vnode_ops device_vnode_ops;

/*
 * Register a device under a name
 * @param name, the name of the device
 * @param vn, the node represeting the device
 * @returns 0 on success, else 1
 */
int device_register(char *name, vnode *vn);

/*
 * Lookup a device
 * O(N) search through the linked list of known devices
 * @param name, the name of the device
 * @param create_file, file creation for a device not supported
 * @param[out] ret, the returned vnode
 * @returns 0 on success, else 1
 */
int device_lookup(char *name, int create_file, vnode **ret);

/*
 * List all devices
 * @param[out] list, the list of names
 * @param[out] nfiles, the number of files listed
 * @returns 0 on success, else 1
 */
int device_list(char ***list, size_t *nfiles);

#endif /* _DEVICE_H_ */
