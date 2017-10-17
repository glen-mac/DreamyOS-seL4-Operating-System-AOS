/*
 * Wrappers for the NFS file system
 * 
 * Glenn McGuire & Cameron Lonsdale
 */

#include "sos_nfs.h"

#include <string.h>
#include <network.h>
#include <fcntl.h>
#include <coro/picoro.h>
#include <clock/clock.h>
#include <nfs/nfs.h>
#include <utils/util.h>

/* Operations on the NFS namespace */
static const vnode_ops nfs_dir_ops = {
    .vop_lookup = sos_nfs_lookup, /* Lookup a particular file */
    .vop_list = sos_nfs_list, /* List all files in the directory */
};

/* Operations on an NFS file */
static const vnode_ops nfs_vnode_ops = {
    .vop_open = sos_nfs_open,
    .vop_close = sos_nfs_close,
    .vop_read = sos_nfs_read,
    .vop_write = sos_nfs_write,
    .vop_stat = sos_nfs_stat
};

/* NFS callbacks */
static void sos_nfs_lookup_callback(uintptr_t token, enum nfs_stat status, fhandle_t* fh, fattr_t* fattr);
static void sos_nfs_write_callback(uintptr_t token, enum nfs_stat status, fattr_t *fattr, int count);
static void sos_nfs_read_callback(uintptr_t token, enum nfs_stat status, fattr_t *fattr, int count, void* data);
static void sos_nfs_getattr_callback(uintptr_t token, enum nfs_stat status, fattr_t *fattr);
static void sos_nfs_readdir_callback(uintptr_t token, enum nfs_stat status, int num_files, char* file_names[], nfscookie_t nfscookie);

/* Timer stages to setup the NFS timer callbacks */
static void sos_nfs_timer_second_stage(uint32_t id, void *data);
static void sos_nfs_timer_callback(uint32_t id, void *data);

/* Globals to save the directory entries */
static char **volatile global_dir = NULL;
static volatile size_t global_size = 0;
static volatile nfscookie_t global_cookie = 0;

/* NFS operation struct to pass as the token (used only for read) */
typedef struct {
    coro routine;
    uiovec *iv;
} nfs_cb;

int
sos_nfs_init(void)
{
    /* Create the vnode with directory operations */
    vnode *nfs_mount = vnode_create(NULL, &nfs_dir_ops, 0, 0);
    if (nfs_mount == NULL) {
        LOG_ERROR("Failed to create the vnode");
        return 1;
    }

    /* Two stage deployent of nfs_timeout so as to allow enough delay to enter into event loop */
    if (register_timer(SECONDS(1), sos_nfs_timer_second_stage, NULL) == 0) {
        LOG_ERROR("Failed to register the nfs timer");
        return 1;
    }

    /* Mount this node as a namespace */
    if (vfs_mount(nfs_mount) != 0) {
        LOG_ERROR("Failed to mount the NFS namespace");
        return 1;
    }

    return 0;
}

int
sos_nfs_lookup(char *name, int create_file, vnode **result)
{
    /* Lookup and yield */
    nfs_lookup(&mnt_point, name, sos_nfs_lookup_callback, (uintptr_t)coro_getcur());
    vnode *vn = yield(NULL);

    if (vn == NULL) {
        if (create_file) {
            LOG_INFO("Creating file %s", name);
            const sattr_t file_attr = {
                .mode = 0664, /* Read write for owner and group, read for everyone */
            };
            nfs_create(&mnt_point, name, &file_attr, sos_nfs_lookup_callback, (uintptr_t)coro_getcur());
            vn = yield(NULL);
        } else {
            LOG_ERROR("Lookup for %s failed", name);
            return 1; /* Lookup failed */
        }
    }

    *result = vn;
    return 0;
}

int
sos_nfs_list(char ***list, size_t *nfiles)
{
    int ret_val;
    *nfiles = 0;

    char **big_list = malloc(sizeof(char *));
    if (big_list == NULL) {
        LOG_ERROR("Failed to create big list array");
        return 1;
    }

    /* Read from the directory until you can read no more */
    do {
        if (nfs_readdir(&mnt_point, global_cookie, sos_nfs_readdir_callback, (uintptr_t)coro_getcur()) != RPC_OK) {
            LOG_ERROR("Failed to read from NFS directory");
            return 1;
        }

        if ((ret_val = (int)yield(NULL)) != 0) {
            free(global_dir);
            LOG_ERROR("Readdir callback failed");
            return 1;
        }

        /* Shuffle the new entries into the big list */
        size_t new_size = (*nfiles) + global_size;
        big_list = realloc(big_list, sizeof(char *) * new_size);
        for (int i = 0; i < global_size; i++)
            big_list[*nfiles + i] = global_dir[i];

        /* Update the size and free the temporary directory array */
        *nfiles = new_size;
        free(global_dir);

    } while (global_cookie != 0);

    /* Return the big list of directory entries */
    *list = big_list;
    return 0;
}

int
sos_nfs_open(vnode *vnode, fmode_t mode)
{
    /* Update vnode stats for readers and writers */
    if (mode == O_RDONLY || mode == O_RDWR) {
        vnode->readcount += 1;
        if (mode == O_RDONLY)
            return 0;
    }
    /* O_RDWR or O_WRONLY */
    vnode->writecount += 1;
    return 0;
}

int
sos_nfs_write(vnode *node, uiovec *iov)
{
    int ret;
    seL4_Word total = iov->uiov_len;

    /* Loop to make sure entire page is written, as nfs could break it up into small packets */
    while (iov->uiov_len > 0) {
        if (nfs_write(node->vn_data, iov->uiov_pos, iov->uiov_len, iov->uiov_base, sos_nfs_write_callback, (uintptr_t)coro_getcur()) != RPC_OK) {
            LOG_ERROR("Failed to write to NFS file");
            return -1;
        }

        ret = (int)yield(NULL);
        if (ret == 0 || ret == -1)
            break;

        assert(iov != NULL);

        /* Progress over the data to write */
        iov->uiov_len -= ret;
        iov->uiov_base += ret;
        iov->uiov_pos += ret;
    }

    return total - iov->uiov_len;
}

int
sos_nfs_read(vnode *node, uiovec *iov)
{
    nfs_cb *cb = malloc(sizeof(nfs_cb));
    if (cb == NULL) {
        LOG_ERROR("Error creating callback struct");
        return -1;
    }
    cb->routine = coro_getcur();
    cb->iv = iov;

    int ret;
    seL4_Word total = iov->uiov_len;

    /* Loop to make sure entire data is read, as nfs could break it up into small packets */
    while (iov->uiov_len > 0) {
        if (nfs_read(node->vn_data, iov->uiov_pos, iov->uiov_len, sos_nfs_read_callback, (uintptr_t)cb) != RPC_OK) {
            LOG_ERROR("Error reading from NFS file");
            free(cb);
            return -1;
        }

        ret = (int)yield(NULL);
        if (ret == 0 || ret == -1)
            break;

        assert(iov != NULL);

        /* Progress over the memmory to read into */
        iov->uiov_len -= ret;
        iov->uiov_base += ret;
        iov->uiov_pos += ret;
    }

    free(cb);
    return total - iov->uiov_len;
}

int
sos_nfs_stat(vnode *node, sos_stat_t **stat)
{
    if (nfs_getattr(node->vn_data, sos_nfs_getattr_callback, (uintptr_t)coro_getcur()) != RPC_OK) {
        LOG_ERROR("Error requesting attributes from file");
        return 1;
    }

    if ((*stat = yield(NULL)) == NULL) {
        LOG_ERROR("Invalid status struct returned from callback");
        return 1;
    }

    return 0;
}

int
sos_nfs_close(vnode *node, fmode_t mode)
{
    if (mode == O_RDONLY || mode == O_RDWR) {
        node->readcount -= 1;
        if (mode == O_RDONLY)
            goto close_file;
    }
    /* O_RDWR or O_WRONLY */
    node->writecount -= 1;

    close_file:
        /* Free the handle */
        if (node->readcount > 0 || node->writecount > 0) {
            free(node->vn_data);
            free(node);
        }

    return 0;
}

/*
 * Callback for the lookup function
 * NFS tells us if the file exists
 * We then build a vnode to represent that file
 */
static void
sos_nfs_lookup_callback(uintptr_t token, enum nfs_stat status, fhandle_t* fh, fattr_t* fattr)
{
    vnode *vn = NULL;
    if (status != NFS_OK) {
        LOG_ERROR("Invalid nfs status %d", status);
        goto coro_resume;
    }

    /* Hardcopy the handle, as the memory location becomes invalid */
    void *handle = malloc(sizeof(fhandle_t));
    if (handle == NULL) {
        LOG_ERROR("Failed to create handle for NFS file");
        goto coro_resume;
    }
    memcpy(handle, fh, sizeof(fhandle_t));

    /* Create the vnode given the handle */
    if ((vn = vnode_create(handle, &nfs_vnode_ops, 0, 0)) == NULL) {
        LOG_ERROR("Failed to create vnode for NFS file");
        free(handle);
        goto coro_resume;
    }

    coro_resume:
        resume((coro)token, (void *)vn);
}

/*
 * Callback to read directory entries
 */
static void
sos_nfs_readdir_callback(uintptr_t token, enum nfs_stat status, int num_files, char *file_names[], nfscookie_t nfscookie)
{
    int ret = -1;
    if (status != NFS_OK) {
        LOG_ERROR("Invalid nfs status %d", status);
        goto coro_resume;
    }

    global_size = num_files;
    if ((global_dir = malloc(sizeof(char *) * num_files)) == NULL) {
        LOG_ERROR("Failed creating the global directory array");
        goto coro_resume;
    }

    /* Copy all filenames into our global array */
    for (int i = 0; i < num_files; i++)
        global_dir[i] = strdup(file_names[i]);

    global_cookie = nfscookie;
    ret = 0;
    coro_resume:
        resume((coro)token, (void *)ret);
}


/*
 * Write callback
 * Return number of bytes written
 */
static void
sos_nfs_write_callback(uintptr_t token, enum nfs_stat status, fattr_t *fattr, int count)
{
    int ret = -1;
    if (status != NFS_OK) {
        LOG_ERROR("Invalid nfs status %d", status);
        goto coro_resume;
    }

    ret = count;
    coro_resume:
        resume((coro)token, (void *)ret);
}

/*
 * Read callback
 * Copy the memory into the buffer specified by the iov_base
 */
static void
sos_nfs_read_callback(uintptr_t token, enum nfs_stat status, fattr_t *fattr, int count, void *data)
{
    int ret = -1;

    /* Unwrap the callback data */
    nfs_cb *call_data = (nfs_cb *)token;
    assert(call_data != NULL);

    if (status != NFS_OK) {
        LOG_ERROR("Invalid nfs status %d", status);
        goto coro_resume;
    }

    /* Hardcopy the data into the specified memory region */
    struct iovec *iov = (struct iovec *)call_data->iv;
    memcpy(iov->iov_base, data, count);

    ret = count;
    coro_resume:
        resume(call_data->routine, (void *)ret);
}

/*
 * Get Attributes callback
 * Copy the attributes of the fattr into the sos_stat_t
 */
static void
sos_nfs_getattr_callback(uintptr_t token, enum nfs_stat status, fattr_t *fattr)
{
    sos_stat_t *stat = NULL;
    if (status != NFS_OK) {
        LOG_ERROR("Invalid nfs status %d", status);
        goto coro_resume;
    }

    if ((stat = malloc(sizeof(sos_stat_t))) == NULL) {
        LOG_ERROR("Failed to create status structure");
        goto coro_resume;
    }

    /* Copy back the stat struct attributes */
    stat->st_type = (st_type_t)fattr->type;
    stat->st_fmode = (fmode_t)fattr->mode;
    stat->st_size = (unsigned)fattr->size;

    /* Time conversion */
    stat->st_ctime = (long)(SEC_TO_MS(fattr->ctime.seconds) + US_TO_MS(fattr->ctime.useconds));
    stat->st_atime = (long)(SEC_TO_MS(fattr->atime.seconds) + US_TO_MS(fattr->atime.useconds));

    coro_resume:
        resume((coro)token, (void *)stat);
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
