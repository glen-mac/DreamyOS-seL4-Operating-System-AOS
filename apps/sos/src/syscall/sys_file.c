/*
 * File Syscall handler
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include "sys_file.h"

#include <fcntl.h>
#include <proc/proc.h>
#include <string.h>
#include "syscall.h"
#include <sys/uio.h>
#include <vfs/file.h>
#include <vm/frametable.h>
#include <vm/vm.h>
#include <utils/util.h>

static seL4_Word syscall_do_read_write(seL4_Word access_mode);

seL4_Word
syscall_write(void)
{
    return syscall_do_read_write(ACCESS_WRITE);
}

seL4_Word
syscall_read(void)
{
    return syscall_do_read_write(ACCESS_READ);
}

seL4_Word
syscall_open(void)
{
    LOG_INFO("syscall: thread made sos_open");

    // TODO: Hard copy the filename because it might cross a page boundary

    seL4_Word name = vaddr_to_sos_vaddr(seL4_GetMR(1), ACCESS_READ);
    fmode_t mode = seL4_GetMR(2);
    LOG_INFO("sycall: thread made open(%s, %d)", (char *)name, mode);

    int result;

    int fd;
    if ((result = fdtable_get_unused_fd(curproc->file_table, &fd)) != 0) {
        result = -1;
        goto message_reply;
    }

    file *open_file;
    if ((result = file_open((char *)name, mode, &open_file) != 0)) {
        result = -1;
        goto message_reply;
    }

    fdtable_insert(curproc->file_table, fd, open_file);
    result = fd;

    message_reply:
        seL4_SetMR(0, result);
        return 1; /* nwords in message */
}

seL4_Word
syscall_close(void)
{
    int fd = seL4_GetMR(1);
    LOG_INFO("thread made close(%d)", fd);

    file *open_file;
    int ret_val;
    if ((ret_val = fdtable_close_fd(curproc->file_table, fd, &open_file)) == 0)
        file_close(open_file);

    seL4_SetMR(0, ret_val);
    return 1; /* nwords in message */
}

static seL4_Word
syscall_do_read_write(seL4_Word access_mode)
{
    int result;

    seL4_Word fd = seL4_GetMR(1);
    seL4_Word buf = seL4_GetMR(2);
    seL4_Word nbytes = seL4_GetMR(3);

    LOG_INFO("syscall: %s(%d, %p, %d) received on SOS", access_mode == ACCESS_READ ? "read": "write", fd, (void *)buf, nbytes);

    file *open_file;
    if ((result = fdtable_get(curproc->file_table, fd, &open_file)) != 0) {
        LOG_ERROR("ftable_get error");
        result = -1;
        goto message_reply;
    }

    if ((access_mode == ACCESS_WRITE && open_file->mode == O_RDONLY) ||
        (access_mode == ACCESS_READ && open_file->mode == O_WRONLY)) {
        LOG_ERROR("Permission error");
        result = -1;
        goto message_reply;
    }

    vnode *vn = open_file->vn;
    seL4_Word bytes_this_round = 0;
    seL4_Word nbytes_remaining = nbytes;
    seL4_Word kvaddr;

    while (nbytes_remaining > 0) {
        if (!(kvaddr = vaddr_to_sos_vaddr(buf, !access_mode))) {
            LOG_ERROR("Address translation failed");
            result = -1;
            goto message_reply;
        }

        bytes_this_round = MIN((PAGE_ALIGN_4K(buf) + PAGE_SIZE_4K) - buf, nbytes_remaining);
        uiovec iov = {
            .uiov_base = (char *)kvaddr,
            .uiov_len = bytes_this_round,
            .uiov_pos = open_file->fp
        };

        if (access_mode == ACCESS_READ) {
            if ((result = vn->vn_ops->vop_read(vn, &iov)) == -1)
                goto message_reply;

            /*
             * Difference between amount read and requested
             * Since packetisation is handled by the concrete implementations,
             * This can only be a device specifying to exit early
             * For example the console reading a new line. Or cat reading the end of a file
             */
            if (result != bytes_this_round) {
                LOG_INFO("Early exit, %d %d", result, bytes_this_round);
                nbytes_remaining -= result;
                open_file->fp += result;
                break;
            }
        } else {
            if ((result = vn->vn_ops->vop_write(vn, &iov)) == -1)
                goto message_reply;
        }

        nbytes_remaining -= result;
        buf += result;
        open_file->fp += result;
    }

    result = nbytes - nbytes_remaining;
    message_reply:
        seL4_SetMR(0, result);
        return 1;
}

seL4_Word
syscall_stat(void)
{
    LOG_INFO("syscall: thread made sos_stat");

    int result = -1;

    seL4_Word name = seL4_GetMR(1);
    seL4_Word stat_buf = seL4_GetMR(2);
    
    seL4_Word kname = vaddr_to_sos_vaddr(name, ACCESS_READ); // TODO: Copy this in, it could cross page boundary, same for open() and others that use name
    seL4_Word kbuf = vaddr_to_sos_vaddr(stat_buf, ACCESS_WRITE); // same

    sos_stat_t *kstat;
    if (vfs_stat((char *)kname, &kstat) != 0)
        goto message_reply;

    // then copy out to the one specified by the user (using our page boundary special copy)
    memcpy((sos_stat_t *)kbuf, (sos_stat_t *)kstat, sizeof(sos_stat_t)); // fix this
    result = 0;
    free(kstat);

    message_reply:
        seL4_SetMR(0, result);
        return 1;
}

seL4_Word
syscall_listdir(void)
{
    LOG_INFO("syscall: thread made sos_getdirent");

    int result = -1;

    seL4_Word pos = seL4_GetMR(1);
    seL4_Word uname = vaddr_to_sos_vaddr(seL4_GetMR(2), ACCESS_WRITE);
    seL4_Word nbytes = seL4_GetMR(3);

    char **dir;
    size_t nfiles = 0;
    if (vfs_list(&dir, &nfiles) != 0)
        goto message_reply;

    /* 1 past the end so we return 0 */
    if (pos == nfiles) {
        result = 0;
        goto message_reply;
    }

    /* Invalid directory entry */
    if (!ISINRANGE(0, pos, nfiles - 1))
        goto message_reply;

    /* Copy the name into the user process */
    size_t bytes_returned = MIN(nbytes, strlen(dir[pos])) + 1; /* + 1 for the null terminator */
    memcpy((void *)uname, (void *)dir[pos], bytes_returned);
    result = bytes_returned;

    message_reply:
        free(dir);
        seL4_SetMR(0, result);
        return 1;
}
