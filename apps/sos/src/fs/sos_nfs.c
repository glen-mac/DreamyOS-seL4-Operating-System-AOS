 
/*
 * Wrappers for the NFS file system
 * 
 * Glenn McGuire & Cameron Lonsdale
 */

#include "sos_nfs.h"

#include <string.h>

#include <network.h>

#include <coro/picoro.h>
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
    .vop_stat = sos_nfs_stat
};

static void sos_nfs_lookup_callback(uintptr_t token, enum nfs_stat status, fhandle_t* fh, fattr_t* fattr);
static void sos_nfs_write_callback(uintptr_t token, enum nfs_stat status, fattr_t *fattr, int count);
static void sos_nfs_read_callback(uintptr_t token, enum nfs_stat status, fattr_t *fattr, int count, void* data);
static void sos_nfs_getattr_callback(uintptr_t token, enum nfs_stat status, fattr_t *fattr);
static void sos_nfs_readdir_callback(uintptr_t token, enum nfs_stat status, int num_files, char* file_names[], nfscookie_t nfscookie);

static void sos_nfs_timer_second_stage(uint32_t id, void *data);
static void sos_nfs_timer_callback(uint32_t id, void *data);

/* Globals to save the directory entries */
static char ** volatile global_dir = NULL;
static volatile size_t global_size = 0;
static volatile nfscookie_t global_cookie = 0;

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
    nfs_lookup(&mnt_point, name, sos_nfs_lookup_callback, (uintptr_t)NULL);
    vn = yield(NULL);

    if (!vn) {
        if (create_file) {
            LOG_INFO("creating file %s", name);
            const sattr_t file_attr = {
                .mode = 0664, /* Read write for owner and group, read for everyone */
                // TODO: creation time and stuff
            };
            nfs_create(&mnt_point, name, &file_attr, sos_nfs_lookup_callback, (uintptr_t)NULL);
            vn = yield(NULL);
        } else {
            LOG_ERROR("lookup for %s failed", name);
            return 1; /* Lookup failed */
        }
    }

    *result = vn;
    return 0;
}

int
sos_nfs_list(char ***list, size_t *nfiles)
{
    *nfiles = 0;

    char **big_list = malloc(sizeof(char *));
    if (!big_list)
        return 1;

    do {
        if (nfs_readdir(&mnt_point, global_cookie, sos_nfs_readdir_callback, (uintptr_t)NULL) != RPC_OK)
            return 1;

        int *ret_val = yield(NULL);
        if (*ret_val != 0) {
            free(global_dir);
            return 1;
        }

        size_t new_size = (*nfiles) + global_size;
        big_list = realloc(big_list, sizeof(char *) * new_size);
        for (int i = 0; i < global_size; i++)
            big_list[*nfiles + i] = global_dir[i];

        *nfiles = new_size;
        free(global_dir);

    } while (global_cookie != 0);

    *list = big_list;
    return 0;
}

int
sos_nfs_write(vnode *node, uiovec *iov)
{
    if (nfs_write(node->vn_data, iov->uiov_pos, iov->uiov_len, iov->uiov_base, sos_nfs_write_callback, (uintptr_t)NULL) != RPC_OK)
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
sos_nfs_stat(vnode *node, sos_stat_t **stat)
{
    if (nfs_getattr(node->vn_data, sos_nfs_getattr_callback, (uintptr_t)NULL) != RPC_OK)
        return -1;

    if ((*stat = yield(NULL)) == NULL)
        return -1;

    return 0;
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
static void
sos_nfs_lookup_callback(uintptr_t token, enum nfs_stat status,
                                fhandle_t* fh, fattr_t* fattr)
{
    vnode *vn = NULL;
    if (status != NFS_OK) {
        LOG_ERROR("lookup staus %d", status);
        goto coro_resume;
    }

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
 * Callback to read directory entries
 */
static void
sos_nfs_readdir_callback(uintptr_t token, enum nfs_stat status, int num_files, char* file_names[], nfscookie_t nfscookie)
{
    int ret_val = -1;
    if (status != NFS_OK) {
        LOG_ERROR("readdir error status: %d", status);
        goto coro_resume;
    }

    global_size = num_files;
    global_dir = malloc(sizeof(char *) * num_files);
    if (!global_dir)
        goto coro_resume;

    for (int i = 0; i < num_files; i++)
        global_dir[i] = strdup(file_names[i]);

    global_cookie = nfscookie;

    ret_val = 0;
    coro_resume:
        resume(syscall_coro, &ret_val);
}


/*
 * Write callback
 * Return number of bytes written
 */
static void
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
static void
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
static void
sos_nfs_getattr_callback(uintptr_t token, enum nfs_stat status, fattr_t *fattr)
{
    sos_stat_t *stat = NULL;
    if (status != NFS_OK) {
        LOG_ERROR("getattr error status: %d", status);
        goto coro_resume;
    }

    stat = malloc(sizeof(sos_stat_t));
    if (!stat) {
        LOG_ERROR("malloc returned NULL");
        goto coro_resume;
    }

    /* Copy back the stat struct attributes */
    stat->st_type = (st_type_t)fattr->type;
    stat->st_fmode = (fmode_t)fattr->mode;
    stat->st_size = (unsigned)fattr->size;

    /* time conversion */
    stat->st_ctime = (long)(fattr->ctime.seconds*1000 + fattr->ctime.useconds / 1000);
    stat->st_atime = (long)(fattr->atime.seconds*1000 + fattr->atime.useconds / 1000);

    coro_resume:
        resume(syscall_coro, stat);
}

/*
 * Second stage to register the repeating timer 
 * As 100ms delay is not enough for SOS to enter into the event loop
 */
static void
sos_nfs_timer_second_stage(uint32_t id, void *data)
{
    assert(register_repeating_timer(MILLISECONDS(100), sos_nfs_timer_callback, NULL) != 0);
}

/*
 * Timer callback to call nfs_timeout
 */
static void
sos_nfs_timer_callback(uint32_t id, void *data)
{
    nfs_timeout();
}
