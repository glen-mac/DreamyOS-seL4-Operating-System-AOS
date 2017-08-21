/*
 * Virtual File System
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#ifndef _VFS_H_
#define _VFS_H_

#include <sel4/sel4.h>

#include <sos.h>

typedef struct {
    seL4_Word dummy;
} vnode;


int vfs_open(char *path, fmode_t mode, vnode **ret);
void vfs_close(vnode *vn);

#endif /* _VFS_H_ */
