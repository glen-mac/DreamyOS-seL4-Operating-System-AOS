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

void
syscall_close(seL4_CPtr reply_cap)
{
    LOG_INFO("syscall: thread made sos_open");
    seL4_MessageInfo_t reply;

    int fd = seL4_GetMR(1);
    int ret_val = 0;

    file * open_file;
    if (fdtable_close_fd(curproc->file_table, fd, &open_file) != 0) {
        ret_val = -1;
        goto message_reply;
    }

    message_reply:
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
    seL4_Word offset = (path_vptr & PAGE_MASK_4K);
    seL4_Word page_id = PAGE_ALIGN_4K(path_vptr);

    seL4_ARM_Page cap;
    fmode_t mode = seL4_GetMR(2); 

    LOG_INFO(">>> path vptr is %p", path_vptr);
    LOG_INFO(">>> path offset is %p", offset);
    LOG_INFO(">>> path page id is %p", page_id);
    LOG_INFO(">>> open mode is %d", mode);

    /* --------------- copy in the path name ----------------- */
    assert(page_directory_lookup(curproc->p_addrspace->directory, page_id, &cap) == 0);
    // cap = second_level[table_index].page;
    LOG_INFO("cap is %p", cap);
    seL4_ARM_Page_GetAddress_t paddr_obj = seL4_ARM_Page_GetAddress(cap);
    LOG_INFO("paddr is %p", paddr_obj.paddr);
    seL4_Word kvaddr = frame_table_paddr_to_sos_vaddr(paddr_obj.paddr + offset);
    /* physical addr is the page frame + offset */
    LOG_INFO(">>> open(%s, %d) received on SOS\n", kvaddr, mode);
    /* --------------- copy in the path name ----------------- */

    /* TODO: grab & check data */
    int result;

    int fd;
    if ((result = fdtable_get_unused_fd(curproc->file_table, &fd)) != 0) {
        fd = result;
        goto message_reply;
        return;
    }

    LOG_INFO("getting fd %d", fd);

    file *open_file;
    if ((result = file_open((char *)kvaddr, mode, &open_file) != 0)) {
        fd = result;
        goto message_reply;
        return;
    }

    fdtable_insert(curproc->file_table, fd, open_file);

    message_reply:
        reply = seL4_MessageInfo_new(0, 0, 0, 1);
        seL4_SetMR(0, fd);
        seL4_Send(reply_cap, reply);

    return;
}


void
syscall_write(seL4_CPtr reply_cap)
{
    seL4_MessageInfo_t reply;
    int result;
    LOG_INFO("syscall: thread made sos_write");

    int fd = 0; /* HACK */

    file *open_file;
    if ((result = fdtable_get(curproc->file_table, fd, &open_file)) != 0) {
        LOG_ERROR("TODO: send ftabale_get error back");
        goto message_reply;
    }

    if (!(open_file->mode == O_WRONLY || open_file->mode == O_RDWR)) {
        LOG_ERROR("TODO: send permission error back to user");
        goto message_reply;
    }

    char *string = "test string internal";
    struct iovec iov = { .iov_base = string, .iov_len = strlen(string) };
    vnode *vn = open_file->vn;
    result = vn->vn_ops->vop_write(vn, &iov);

    message_reply:
        reply = seL4_MessageInfo_new(0, 0, 0, 1);
        seL4_SetMR(0, result);
        seL4_Send(reply_cap, reply);
}
