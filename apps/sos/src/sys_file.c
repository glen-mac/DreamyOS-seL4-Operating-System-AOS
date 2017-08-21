/*
 * File Syscall handler
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include "syscall.h"
#include "sys_file.h"
#include <sos.h>
#include <cspace/cspace.h>
#include <utils/util.h>
#include <serial/serial.h>

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
