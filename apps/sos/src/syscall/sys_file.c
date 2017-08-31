/*
 * File Syscall handler
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include "syscall.h"
#include "sys_file.h"
#include "proc.h"
#include "frametable.h"
#include <vfs/file.h>
#include <fcntl.h>

#include <sys/panic.h>
#include <sys/uio.h>

#include <vm/vm.h>

#include <sos.h>
#include <cspace/cspace.h>
#include <utils/util.h>
#include <serial/serial.h>

#include <string.h>

static void syscall_do_read_write(seL4_CPtr reply_cap, seL4_Word access_mode);

/* helper func to convert a vaddr to kvaddr */
static seL4_Word vaddr_to_kvaddr(seL4_Word vaddr, seL4_Word access_type);

void
syscall_write(seL4_CPtr reply_cap)
{
    LOG_INFO("syscall: thread made sos_write");
    syscall_do_read_write(reply_cap, ACCESS_WRITE);
}

void
syscall_read(seL4_CPtr reply_cap)
{
    LOG_INFO("syscall: thread made sos_read");
    syscall_do_read_write(reply_cap, ACCESS_READ);
}

void
syscall_open(seL4_CPtr reply_cap)
{
    LOG_INFO("syscall: thread made sos_open");

    seL4_MessageInfo_t reply;

    seL4_Word kvaddr = vaddr_to_kvaddr(seL4_GetMR(1), ACCESS_READ); /* read from path */
    fmode_t mode = seL4_GetMR(2);
    LOG_INFO("sycall: open(%s, %d) received on SOS", (char *)kvaddr, mode);

    /* TODO: grab & check data */
    int result;

    int fd;
    if ((result = fdtable_get_unused_fd(curproc->file_table, &fd)) != 0) {
        result = -1;
        goto message_reply;
    }

    file *open_file;
    if ((result = file_open((char *)kvaddr, mode, &open_file) != 0)) {
        result = -1;
        goto message_reply;
    }

    fdtable_insert(curproc->file_table, fd, open_file);
    result = fd;

    message_reply:
        reply = seL4_MessageInfo_new(0, 0, 0, 1);
        seL4_SetMR(0, result);
        seL4_Send(reply_cap, reply);
}

void
syscall_close(seL4_CPtr reply_cap)
{
    LOG_INFO("syscall: thread made sos_close");
    seL4_MessageInfo_t reply;

    int ret_val;
    int fd = seL4_GetMR(1);

    file *open_file;
    ret_val = fdtable_close_fd(curproc->file_table, fd, &open_file);

    reply = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, ret_val);
    seL4_Send(reply_cap, reply);
}

static void
syscall_do_read_write(seL4_CPtr reply_cap, seL4_Word access_mode)
{
    seL4_MessageInfo_t reply;

    int result;
    seL4_Word kvaddr;

    seL4_Word fd = seL4_GetMR(1);
    seL4_Word buf = seL4_GetMR(2);
    seL4_Word nbytes = seL4_GetMR(3);

    seL4_Word nbytes_remaining = nbytes;

    LOG_INFO("syscall: %s(%d, %p, %d) received on SOS", access_mode == ACCESS_READ ? "read": "write", fd, (void *)buf, nbytes);

    file *open_file;
    if ((result = fdtable_get(curproc->file_table, fd, &open_file)) != 0) {
        LOG_ERROR("ftable_get error");
        result = -1;
        goto message_reply;
    }

    if ((access_mode == ACCESS_WRITE && !(open_file->mode == O_WRONLY || open_file->mode == O_RDWR)) ||
        (access_mode == ACCESS_READ && !(open_file->mode == O_RDONLY || open_file->mode == O_RDWR))) {
        LOG_ERROR("permission error");
        result = -1;
        goto message_reply;
    }

    vnode *vn = open_file->vn;
    int bytes_this_round = 0;

    while (nbytes_remaining != 0) {
        if (!(kvaddr = vaddr_to_kvaddr(buf, !access_mode))) {
            /* Could not translate address */ 
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
            result = vn->vn_ops->vop_read(vn, &iov);

            /* Didnt read enough data, could be packet loss OR newline. Could be a problem in the future idk */
            if (result != bytes_this_round) {
                nbytes_remaining -= result;
                LOG_INFO("Early exit");
                break;
            }
        } else {
            result = vn->vn_ops->vop_write(vn, &iov);
        }

        nbytes_remaining -= result;
        buf += result;
    }

    result = nbytes - nbytes_remaining;

    /* Increment the file pointer */
    if (result > 0)
        open_file->fp += result;

    message_reply:
        reply = seL4_MessageInfo_new(0, 0, 0, 1);
        seL4_SetMR(0, result);
        seL4_Send(reply_cap, reply);
}

void
syscall_stat(seL4_CPtr reply_cap)
{
    LOG_INFO("syscall: thread made sos_stat");

    int result = -1;

    seL4_Word name = seL4_GetMR(1);
    seL4_Word stat_buf = seL4_GetMR(2);
    
    seL4_Word kname = vaddr_to_kvaddr(name, ACCESS_READ); // TODO: Copy this in, it could cross page boundary, same for open() and others that use name
    seL4_Word kbuf = vaddr_to_kvaddr(stat_buf, ACCESS_WRITE); // same

    sos_stat_t *kstat = malloc(sizeof(sos_stat_t));
    if (!kstat)
        goto message_reply;

    if (vfs_stat((char *)kname, (sos_stat_t *)kstat) != 0)
        goto message_reply;

    // then copy out to the one specified by the user (using our page boundary special copy)
    memcpy((sos_stat_t *)kbuf, (sos_stat_t *)kstat, sizeof(sos_stat_t)); // fix this

    result = 0;
    message_reply:
        free(kstat);
        seL4_MessageInfo_t reply = seL4_MessageInfo_new(0, 0, 0, 1);
        seL4_SetMR(0, result);
        seL4_Send(reply_cap, reply);
}

void
syscall_listdir(seL4_CPtr reply_cap)
{   
    seL4_MessageInfo_t reply;

    LOG_INFO("syscall: thread made sos_getdirent");

    int result = -1;

    seL4_Word pos = seL4_GetMR(1);
    seL4_Word uname = vaddr_to_kvaddr(seL4_GetMR(2), ACCESS_WRITE);
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
    size_t bytes_returned = MIN(nbytes, strlen(dir[pos]));
    memcpy(uname, dir[pos], bytes_returned);
    result = bytes_returned;

    free(dir);

    message_reply:
        reply = seL4_MessageInfo_new(0, 0, 0, 1);
        seL4_SetMR(0, result);
        seL4_Send(reply_cap, reply);
}

static seL4_Word
vaddr_to_kvaddr(seL4_Word vaddr, seL4_Word access_type)
{
    int err;

    seL4_Word offset = (vaddr & PAGE_MASK_4K);
    seL4_Word page_id = PAGE_ALIGN_4K(vaddr);
    seL4_ARM_Page cap;

    addrspace *as = curproc->p_addrspace;

    /* Check stack expansion */
    region *vaddr_region;
    if (as_find_region(as, vaddr, &vaddr_region) != 0 &&
        as_region_collision_check(as, PAGE_ALIGN_4K(vaddr), as->region_stack->vaddr_end) == 0) {
        as->region_stack->vaddr_start = PAGE_ALIGN_4K(vaddr);
    }
    
    /* Check if address belongs to a region and that region has permissions for the access type */
    if (as_find_region(as, vaddr, &vaddr_region) != 0 ||
        !as_region_permission_check(vaddr_region, access_type)) {
        return (seL4_Word)NULL; /* Invalid region, or incorrect permissions */
    }

    /* Valid region with valid permissions */
    /* Check if the page is mapped into physical memory yet */
    seL4_Word kvaddr;
    err = page_directory_lookup(curproc->p_addrspace->directory, page_id, &cap);
    if (err) {
        LOG_ERROR("lookup failed");
        /* Try to map it in */
        if (vm_try_map(page_id, access_type, &kvaddr) != 0) {
            LOG_ERROR("map failed");
            return (seL4_Word)NULL;
        }

        cap = frame_table_get_capability(frame_table_sos_vaddr_to_index(kvaddr));
    }
    seL4_ARM_Page_GetAddress_t paddr_obj = seL4_ARM_Page_GetAddress(cap);
    kvaddr = frame_table_paddr_to_sos_vaddr(paddr_obj.paddr + offset);
    return kvaddr;
}
