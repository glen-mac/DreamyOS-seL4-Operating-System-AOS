/*
 * Virtual File System
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include "vfs.h"

#include <assert.h>

int
vfs_open(char *path, fmode_t mode, vnode **ret)
{
    assert(!"Not implemented");
    return 1;
}

void
vfs_close(vnode *vn)
{
    assert(!"Not implemented");
    return 1;
}
