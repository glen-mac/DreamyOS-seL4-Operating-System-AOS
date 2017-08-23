/*
 * File Syscall handler
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include "syscall.h"
#include "sys_file.h"
#include "vm.h"
#include "vfs.h"
#include "proc.h"
#include <sos.h>
#include <cspace/cspace.h>
#include <utils/util.h>
#include <serial/serial.h>

void
syscall_open(seL4_CPtr reply_cap) {
    LOG_INFO("syscall: thread made sos_open");
    seL4_MessageInfo_t reply;

    uint8_t * path_vptr = (uint8_t *)seL4_GetMR(1);
    uint8_t * path_vptr_a = (uint8_t *)PAGE_ALIGN_4K((seL4_Word)path_vptr);
    uint8_t * path_pptr;
    seL4_CPtr cap;
    int mode = seL4_GetMR(2); 

    LOG_INFO(">>> open vptr is %p\n", seL4_GetMR(1));
    LOG_INFO(">>> open vptr_a is %p\n", path_vptr_a);
    LOG_INFO(">>> open mode is %d\n", mode);
    /* --------------- copy in the path name ----------------- */
    /* get the indices for lookup */
    seL4_Word directory_index = DIRECTORY_INDEX((seL4_Word)path_vptr_a);
    seL4_Word table_index = TABLE_INDEX((seL4_Word)path_vptr_a);
    /* grab the page table directory */
    seL4_Word *directory = curproc->p_addrspace->directory;
    /* get the second level*/
    LOG_INFO(">>> directory_index: %x\n", directory_index);
    LOG_INFO(">>> table_index: %x\n", table_index);
    page_table_entry *second_level = (page_table_entry *)directory[directory_index];
    assert(second_level[table_index].page != (seL4_CPtr)NULL);
    cap = second_level[table_index].page;
    seL4_ARM_Page_GetAddress_t paddr_obj = seL4_ARM_Page_GetAddress(cap);
    /* physical addr is the page frame + offset */
    seL4_Word paddr = paddr_obj.paddr + (seL4_Word)(path_vptr - path_vptr_a);
    path_pptr = (uint8_t *)paddr;
    LOG_INFO(">>> open(%s, %d) received on SOS\n", path_pptr, mode);
    /* --------------- copy in the path name ----------------- */

    /* call vfs_open */
    int ret_fd = 1; //vfs_open(path_pptr, mode);
    
    reply = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, ret_fd);
    seL4_Send(reply_cap, reply);
}


void
syscall_write(seL4_CPtr reply_cap, void * message, struct serial *serial_port) {
    seL4_MessageInfo_t reply;
    
    LOG_INFO("syscall: thread made sos_write");

    size_t max_msg_size = (seL4_MsgMaxLength - 2) * sizeof(seL4_Word);
    size_t nbytes = seL4_GetMR(1);

    if (nbytes > max_msg_size)
        nbytes = max_msg_size;

    /* 
     * Byte string of characters, 4 characters in one word 
     * Skip over the nbytes field 
     */
    char *buffer = (char *)(message + sizeof(seL4_Word));

    /* Send to serial and reply with how many bytes were sent */
    nbytes = serial_send(serial_port, buffer, nbytes);
    reply = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, nbytes);
    seL4_Send(reply_cap, reply);

}
