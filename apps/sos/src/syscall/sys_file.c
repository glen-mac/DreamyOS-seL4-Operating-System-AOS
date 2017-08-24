/*
 * File Syscall handler
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include "syscall.h"
#include "sys_file.h"
#include "proc.h"
#include <vfs/file.h>
#include <fcntl.h>

#include <sys/panic.h>
#include <sys/uio.h>

#include <sos.h>
#include <cspace/cspace.h>
#include <utils/util.h>
#include <serial/serial.h>

/* helper func to convert a vaddr to kvaddr */
static seL4_Word vaddr_to_kvaddr(seL4_Word vaddr);

void
syscall_close(seL4_CPtr reply_cap)
{
    LOG_INFO("syscall: thread made sos_close");
    seL4_MessageInfo_t reply;

    int ret_val;
    int fd = seL4_GetMR(1);

    file * open_file;
    ret_val = fdtable_close_fd(curproc->file_table, fd, &open_file);

    reply = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, ret_val);
    seL4_Send(reply_cap, reply);
}

void
syscall_open(seL4_CPtr reply_cap)
{
    LOG_INFO("syscall: thread made sos_open");
    seL4_MessageInfo_t reply;

    seL4_Word path_vptr = seL4_GetMR(1);
    seL4_Word kvaddr = vaddr_to_kvaddr(path_vptr);
    fmode_t mode = seL4_GetMR(2); 
    LOG_INFO(">>> open(%s, %d) received on SOS\n", kvaddr, mode);

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

    LOG_INFO("result is %d", result);
}

void
syscall_read(seL4_CPtr reply_cap)
{
    seL4_MessageInfo_t reply;
    int result;
    LOG_INFO("syscall: thread made sos_read");

    int fd = 0; /* HACK */

    file *open_file;
    if ((result = fdtable_get(curproc->file_table, fd, &open_file)) != 0) {
        LOG_ERROR("TODO: send ftable_get error back");
        goto message_reply;
    }

    if (!(open_file->mode == O_RDONLY || open_file->mode == O_RDWR)) {
        LOG_ERROR("TODO: send permission error back to user");
        goto message_reply;
    }

    seL4_Word file = seL4_GetMR(1);
    seL4_Word buf = seL4_GetMR(2);
    seL4_Word nbyte = seL4_GetMR(3);
    seL4_Word kvaddr = vaddr_to_kvaddr(buf);
    LOG_INFO(">>> read(%d, %x, %d) received on SOS\n", file, buf, nbyte);

    struct iovec iov = { .iov_base = kvaddr, .iov_len = nbyte };
    vnode *vn = open_file->vn;
    result = vn->vn_ops->vop_read(vn, &iov);

    message_reply:
        reply = seL4_MessageInfo_new(0, 0, 0, 1);
        seL4_SetMR(0, result);
        seL4_Send(reply_cap, reply);
}

void
syscall_write(seL4_CPtr reply_cap)
{
    seL4_MessageInfo_t reply;
    int result;
    LOG_INFO("syscall: thread made sos_write");

    seL4_Word fd = seL4_GetMR(1);
    seL4_Word buf = seL4_GetMR(2);
    seL4_Word nbytes = seL4_GetMR(3);
    seL4_Word kvaddr = vaddr_to_kvaddr(buf);
    LOG_INFO(">>> write(%d, %x, %d) received on SOS\n", fd, buf, nbytes);

    file *open_file;
    if ((result = fdtable_get(curproc->file_table, fd, &open_file)) != 0) {
        LOG_ERROR("ftabale_get error");
        result = -1;
        goto message_reply;
    }

    if (!(open_file->mode == O_WRONLY || open_file->mode == O_RDWR)) {
        LOG_ERROR("permission error %d", open_file->mode);
        result = -1;
        goto message_reply;
    }

    struct iovec iov = { .iov_base = kvaddr, .iov_len = nbytes };
    vnode *vn = open_file->vn;
    result = vn->vn_ops->vop_write(vn, &iov);

    message_reply:
        reply = seL4_MessageInfo_new(0, 0, 0, 1);
        seL4_SetMR(0, result);
        seL4_Send(reply_cap, reply);
}

static seL4_Word
vaddr_to_kvaddr(seL4_Word vaddr)
{
    seL4_Word offset = (vaddr & PAGE_MASK_4K);
    seL4_Word page_id = PAGE_ALIGN_4K(vaddr);
    seL4_ARM_Page cap;
    LOG_INFO(">>> vptr is %p", vaddr);
    LOG_INFO(">>> offset is %p", offset);
    LOG_INFO(">>> page id is %p", page_id);
    assert(page_directory_lookup(curproc->p_addrspace->directory, page_id, &cap) == 0);
    LOG_INFO(">>> cap is %p", cap);
    seL4_ARM_Page_GetAddress_t paddr_obj = seL4_ARM_Page_GetAddress(cap);
    LOG_INFO(">>> paddr is %p", paddr_obj.paddr);
    seL4_Word kvaddr = frame_table_paddr_to_sos_vaddr(paddr_obj.paddr + offset);
    return kvaddr;
}
