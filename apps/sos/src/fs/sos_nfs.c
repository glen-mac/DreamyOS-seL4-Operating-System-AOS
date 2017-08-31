 
/*
 * Wrappers for the NFS file system
 * 
 * Glenn McGuire & Cameron Lonsdale
 */

#include "sos_nfs.h"

#include <string.h>

#include <network.h>

#include "picoro.h"
#include <syscall/syscall.h>


#include <clock/clock.h>
#include <lwip/ip_addr.h>
#include <nfs/nfs.h>

#include <utils/util.h>
#include <utils/time.h>

static const vnode_ops nfs_dir_ops = {
    .vop_lookup = sos_nfs_lookup,
    .vop_list = sos_nfs_list,
};

static const vnode_ops nfs_vnode_ops = {
    .vop_close = sos_nfs_close,
    .vop_read = sos_nfs_read,
    .vop_write = sos_nfs_write,
};

static nfs_lookup_cb_t sos_nfs_lookup_callback(uintptr_t token, enum nfs_stat status, fhandle_t* fh, fattr_t* fattr);
static nfs_write_cb_t sos_nfs_write_callback(uintptr_t token, enum nfs_stat status, fattr_t *fattr, int count);
static nfs_read_cb_t sos_nfs_read_callback(uintptr_t token, enum nfs_stat status, fattr_t *fattr, int count, void* data);
static nfs_getattr_cb_t sos_nfs_getattr_callback(uintptr_t token, enum nfs_stat status, fattr_t *fattr);

static timer_callback_t sos_nfs_timer_second_stage(uint32_t id, void *data);
static timer_callback_t sos_nfs_timer_callback(uint32_t id, void *data);

int
sos_nfs_init(void)
{
    vnode *nfs_mount = malloc(sizeof(vnode));
    if (!nfs_mount) {
        LOG_ERROR("malloc failed when creating nfs namespace");
        return 1; 
    }

    /* Two stage deployent of nfs_timeout so as to allow enough delay to enter into event loop */
    if (register_timer(SECONDS(1), sos_nfs_timer_second_stage, NULL) == 0) {
        LOG_ERROR("registering timer failed");
        return 1;
    }

    nfs_mount->vn_ops = &nfs_dir_ops;
    vfs_mount(nfs_mount);

    return 0;
}

int
sos_nfs_lookup(char *name, int create_file, vnode **result)
{
    vnode *vn;
    nfs_lookup(&mnt_point, name, sos_nfs_lookup_callback, NULL);
    vn = yield(NULL);

    if (!vn && create_file) {
        LOG_INFO("creating file %s", name);
        const sattr_t file_attr = {
            .mode = 0664, /* Read write for owner and group, read for everyone */
            // TODO: creation time and stuff
        };
        nfs_create(&mnt_point, name, &file_attr, (nfs_create_cb_t)sos_nfs_lookup_callback, NULL);
        vn = yield(NULL);
    } else {
        return 1; /* Lookup failed */
    }

    *result = vn;
    return 0;
}

int
sos_nfs_list(char ***list, size_t *nfiles)
{
    
    *nfiles = 0;
    return 0;
}

int
sos_nfs_write(vnode *node, uiovec *iov)
{
    if (nfs_write(node->vn_data, iov->uiov_pos, iov->uiov_len, iov->uiov_base, sos_nfs_write_callback, NULL) != RPC_OK)
        return -1;

    int *ret = yield(NULL);
    return *ret;
}

int
sos_nfs_read(vnode *node, uiovec *iov)
{
    if (nfs_read(node->vn_data, iov->uiov_pos, iov->uiov_len, sos_nfs_read_callback, (uintptr_t)iov) != RPC_OK)
        return -1;

    int *ret = yield(NULL);
    return *ret;
}

int
sos_nfs_getattr(vnode *node, sos_stat_t *stat)
{
    if (nfs_getattr(node->vn_data, sos_nfs_getattr_callback, (uintptr_t)stat) != RPC_OK)
        return -1;

    int *ret = yield(NULL);
    return *ret;
}

int
sos_nfs_close(vnode *node)
{
    /* Free the handle */
    free(node->vn_data);
    free(node);
    return 0;
}

/*
 * Callback for the lookup function
 * NFS tells us if the file exists
 * We then build a vnode to represent that file
 */
static nfs_lookup_cb_t
sos_nfs_lookup_callback(uintptr_t token, enum nfs_stat status,
                                fhandle_t* fh, fattr_t* fattr)
{
    vnode *vn = NULL;
    if (status != NFS_OK)
        goto coro_resume;

    if ((vn = malloc(sizeof(vnode))) == NULL) {
        LOG_ERROR("malloc failed when creating vnode");
        goto coro_resume;
    }

    vn->vn_data = malloc(sizeof(fhandle_t));
    memcpy(vn->vn_data, fh, sizeof(fhandle_t));

    vn->vn_ops = &nfs_vnode_ops;

    coro_resume:
        resume(syscall_coro, vn);
}

/*
 * Write callback
 * Return number of bytes written
 */
static nfs_write_cb_t
sos_nfs_write_callback(uintptr_t token, enum nfs_stat status, fattr_t *fattr, int count)
{
    int ret_val = -1;
    if (status != NFS_OK) {
        LOG_ERROR("write error status: %d", status);
        goto coro_resume;
    }

    ret_val = count;
    coro_resume:
        resume(syscall_coro, &ret_val);
}

/*
 * Read callback
 * Copy the memory into the buffer specified by the iov_base
 */
static nfs_read_cb_t
sos_nfs_read_callback(uintptr_t token, enum nfs_stat status, fattr_t *fattr, int count, void* data)
{
    int ret_val = -1;
    if (status != NFS_OK) {
        LOG_ERROR("read error status: %d", status);
        goto coro_resume;
    }

    struct iovec *iov = (struct iovec *)token;
    memcpy(iov->iov_base, data, count);
    ret_val = count;

    coro_resume:
        resume(syscall_coro, &ret_val);
}


/* Get Attributes callback
 * Copy the attributes of the fattr into the sos_stat_t
 */
static nfs_getattr_cb_t
sos_nfs_getattr_callback(uintptr_t token, enum nfs_stat status, fattr_t *fattr)
{
    int ret_val = -1;
    if (status != NFS_OK) {
        LOG_ERROR("getattr error status: %d", status);
        goto coro_resume;
    }

    ret_val = 0;
    /* copy back the stat struct attributes */
    sos_stat_t *stat = (sos_stat_t *)token;
    stat->st_type = (st_type_t)fattr->type;
    stat->st_fmode = (fmode_t)fattr->mode;
    stat->st_size = (unsigned)fattr->size;
    /* time conversion */
    stat->st_ctime = (long)(fattr->ctime.seconds*1000 + fattr->ctime.useconds / 1000);
    stat->st_atime = (long)(fattr->atime.seconds*1000 + fattr->atime.useconds / 1000);

    coro_resume:
        resume(syscall_coro, &ret_val);
}


/*
 * Second stage to register the repeating timer 
 * As 100ms delay is not enough for SOS to enter into the event loop
 */
static timer_callback_t
sos_nfs_timer_second_stage(uint32_t id, void *data)
{
    assert(register_repeating_timer(MILLISECONDS(100), sos_nfs_timer_callback, NULL) != 0);
}

/*
 * Timer callback to call nfs_timeout
 */
static timer_callback_t
sos_nfs_timer_callback(uint32_t id, void *data)
{
    nfs_timeout();
}
