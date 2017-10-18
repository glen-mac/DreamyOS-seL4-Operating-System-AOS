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

static int syscall_do_read_write(seL4_Word access_mode, proc *curproc);

int
syscall_write(proc *curproc)
{
    return syscall_do_read_write(ACCESS_WRITE, curproc);
}

int
syscall_read(proc *curproc)
{
    return syscall_do_read_write(ACCESS_READ, curproc);
}

int
syscall_open(proc *curproc)
{
    int result;
    int fd;

    /* File name to open */
    seL4_Word name = seL4_GetMR(1);
    /* Mde to open the file in */
    fmode_t mode = seL4_GetMR(2);

    LOG_SYSCALL(curproc->pid, "open(%p, %d)", (void *)name, mode);

    /* Copy the file into a local buffer, as the name may span multiple frames */
    char kname[NAME_MAX];
    if (copy_in(curproc, kname, name, NAME_MAX) != 0) {
        LOG_ERROR("Error copying in filename");
        result = -1;
        goto message_reply;
    }

    /* Explicit null terminate in case one is not provided */
    kname[NAME_MAX - 1] = '\0';

    if ((result = fdtable_get_unused_fd(curproc->file_table, &fd)) != 0) {
        LOG_ERROR("Failed to acquire unused fd");
        result = -1;
        goto message_reply;
    }

    file *open_file;
    if ((result = file_open((char *)kname, mode, &open_file) != 0)) {
        LOG_ERROR("Failed to open file");
        result = -1;
        goto message_reply;
    }

    fdtable_insert(curproc->file_table, fd, open_file);
    result = fd;

    message_reply:
        seL4_SetMR(0, result);
        return 1; /* nwords in message */
}

int
syscall_close(proc *curproc)
{
    file *open_file = NULL;
    int result = -1;

    int fd = seL4_GetMR(1);

    LOG_SYSCALL(curproc->pid, "close(%d)", fd);

    if (fdtable_close_fd(curproc->file_table, fd, &open_file) != 0) {
        LOG_ERROR("Invalid file descriptor");
        goto message_reply;
    }

    file_close(open_file);
    result = 0;
    message_reply:
        seL4_SetMR(0, result);
        return 1; /* nwords in message */
}

int
syscall_stat(proc *curproc)
{
    int result = -1;

    /* Name of the file to stat */
    seL4_Word name = seL4_GetMR(1);
    /* Location of where to put the stat info */
    seL4_Word stat_buf = seL4_GetMR(2);

    LOG_SYSCALL(curproc->pid, "sos_stat(%p, %p)", (void *)name, (void *)stat_buf);  

    /* Copy in name from userland */
    char kname[NAME_MAX];
    if (copy_in(curproc, kname, name, NAME_MAX) != 0) {
        LOG_ERROR("Error copying in filename");
        result = -1;
        goto message_reply;
    }

    /* Explicit null terminate in case one is not provided */
    kname[NAME_MAX - 1] = '\0';

    sos_stat_t *kstat;
    /* Stat the file through the VFS */
    if (vfs_stat((char *)kname, &kstat) != 0) {
        LOG_ERROR("Failed to stat the file");
        goto message_reply;
    }

    /* Copy out stat to user process */
    if (copy_out(curproc, stat_buf, (char *)kstat, sizeof(sos_stat_t)) != 0) {
        LOG_ERROR("Error copying out to userland");
        free(kstat);
        goto message_reply;
    }

    free(kstat);
    result = 0;

    message_reply:
        seL4_SetMR(0, result);
        return 1;
}

int
syscall_listdir(proc *curproc)
{
    int result = -1;

    int pos = seL4_GetMR(1);
    seL4_Word uname = seL4_GetMR(2);
    int nbytes = seL4_GetMR(3);

    LOG_SYSCALL(curproc->pid, "sos_getdirent(%d, %p, %d)", pos, (void *)uname, nbytes);

    if (nbytes <= 0) {
        LOG_ERROR("nbytes must be positive");
        goto message_reply;
    }

    /* Acquire a list of file names */
    char **dir = NULL;
    size_t nfiles = 0;
    if (vfs_list(&dir, &nfiles) != 0) {
        LOG_ERROR("Failed to list files from the VFS");
        goto message_reply;
    }

    /* 1 past the end so we return 0 */
    if (pos == nfiles) {
        free(dir);
        result = 0;
        goto message_reply;
    }

    /* Invalid directory entry */
    if (!ISINRANGE(0, pos, nfiles - 1)) {
        free(dir);
        LOG_ERROR("Invalid directory entry");
        goto message_reply;
    }

    /* Copy out the name to userland buffer */
    size_t bytes_returned = MIN(nbytes, strlen(dir[pos])) + 1;
    if (copy_out(curproc, uname, dir[pos], bytes_returned) != 0) {
        LOG_ERROR("Error copying out to userland");
        free(dir);
        goto message_reply;
    }

    result = bytes_returned;
    free(dir);

    message_reply:
        seL4_SetMR(0, result);
        return 1;
}

/*
 * Perform a read or write operation
 * @param access_mode, operation type
 * @param curproc, the process reuesting
 * @returns nbytes on success, else -1
 */
static int
syscall_do_read_write(seL4_Word access_mode, proc *curproc)
{
    int result = -1;

    seL4_Word fd = seL4_GetMR(1);
    seL4_Word buf = seL4_GetMR(2);
    seL4_Word nbytes = seL4_GetMR(3);

    LOG_SYSCALL(curproc->pid, "%s(%d, %p, %d)", access_mode == ACCESS_READ ? "read": "write", fd, (void *)buf, nbytes);

    file *open_file = NULL;
    if (fdtable_get(curproc->file_table, fd, &open_file) != 0) {
        LOG_ERROR("Failed to retrieve file from fd");
        goto message_reply;
    }

    if ((access_mode == ACCESS_WRITE && open_file->mode == O_RDONLY) ||
        (access_mode == ACCESS_READ && open_file->mode == O_WRONLY)) {
        LOG_ERROR("File doesnt support the requested mode of access");
        goto message_reply;
    }

    vnode *vn = open_file->vn;
    /* The number of bytes in the transaction this round */
    seL4_Word bytes_this_round = 0;
    seL4_Word nbytes_remaining = nbytes;
    seL4_Word kvaddr;

    while (nbytes_remaining > 0) {
        if (!(kvaddr = vaddr_to_sos_vaddr(curproc, buf, !access_mode))) {
            LOG_ERROR("Failed to translate virtual address to sos virtual");
            result = -1; /* Return failure, rather than the ammount succesfully written */
            goto message_reply;
        }

        /*
         * Pin the frame, we dont want it paged out during these operations
         * Recover original chance to restore it after transaction
         */
        enum chance_type original_chance;
        seL4_Word frame_id = frame_table_sos_vaddr_to_index(kvaddr);
        assert(frame_table_get_chance(frame_id, &original_chance) == 0);
        assert(frame_table_set_chance(frame_id, PINNED) == 0);

        /* Minimum between number of bytes left in the frame, or the number of bytes remaning in the operation */
        bytes_this_round = MIN((PAGE_ALIGN_4K(buf) + PAGE_SIZE_4K) - buf, nbytes_remaining);

        /* Setup the io vector with the address, length and position in the file */
        uiovec iov = {
            .uiov_base = (char *)kvaddr,
            .uiov_len = bytes_this_round,
            .uiov_pos = open_file->fp
        };

        if (access_mode == ACCESS_READ) {
            if ((result = vn->vn_ops->vop_read(vn, &iov)) == -1) {
                LOG_ERROR("Failed to read from the vnode");
                assert(frame_table_set_chance(frame_id, original_chance) == 0);
                goto message_reply;
            }

            /*
             * Difference between amount read and requested
             * Since packetisation is handled by the concrete implementations,
             * This can only be a device specifying to exit early
             * For example the console reading a new line. Or cat reading the end of a file
             */
            if (result != bytes_this_round) {
                LOG_INFO("Early exit, returned bytes %d requested %d", result, bytes_this_round);
                nbytes_remaining -= result;
                open_file->fp += result;
                break;
            }
        } else {
            if ((result = vn->vn_ops->vop_write(vn, &iov)) == -1) {
                LOG_ERROR("Failed to write to the vnode");
                assert(frame_table_set_chance(frame_id, original_chance) == 0);
                goto message_reply;
            }
        }

        /* Reset the chance */
        assert(frame_table_set_chance(frame_id, original_chance) == 0);

        /* Shift along to process the remaining data */
        nbytes_remaining -= result;
        buf += result;
        open_file->fp += result;
    }

    /* Return the total amount of data sent / received */
    result = nbytes - nbytes_remaining;

    message_reply:
        seL4_SetMR(0, result);
        return 1;
}
