/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/****************************************************************************
 *
 *      $Id:  $
 *
 *      Description: Simple milestone 0 code.
 *      		     Libc will need sos_write & sos_read implemented.
 *
 *      Author:      Ben Leslie
 *
 ****************************************************************************/

#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "ttyout.h"

#include <sel4/sel4.h>

void ttyout_init(void) {
    /* Perform any initialisation you require here */
}

static size_t sos_debug_print(const void *vData, size_t count) {
    size_t i;
    const char *realdata = vData;
    for (i = 0; i < count; i++)
        seL4_DebugPutChar(realdata[i]);
    return count;
}

/*
 * write count bytes of data to serial port
 *
 */
size_t sos_write(void *data, size_t count) {
    seL4_MessageInfo_t tag;
    size_t nbytes, nwords;

    char *message = data;

    /*
     * Measured in bytes.
     * subtracted by 2 words for syscall and nbytes fields.
     */
    size_t msg_size = (seL4_MsgMaxLength - 2) * sizeof(seL4_Word);
    size_t bytes_remaining = count;
    size_t bytes_sent = 0;

    /*
     * Because count can be larger than message buffer size,
     * we need to break up the message into several messages.
     */
    for (size_t message_id = 0; message_id < (count / msg_size) + 1; ++message_id) {
        /* Work out how many bytes to send in this message */
        nbytes = bytes_remaining > msg_size ? msg_size: bytes_remaining;

        /* 2 comes from the syscall and nbytes fields. A bit of maths to round up to nearest word from nbytes */
        nwords = 2 + (nbytes + sizeof(seL4_Word) - 1) / sizeof(seL4_Word);
        tag = seL4_MessageInfo_new(0, 0, 0, nwords);
        seL4_SetTag(tag);
        seL4_SetMR(0, 1); /* Syscall number */
        seL4_SetMR(1, nbytes); /* Number of bytes in the message */

        /* Copy the message into the IPC buffer past the syscall and nbytes fields */
        memcpy(seL4_GetIPCBuffer()->msg + 2, message + bytes_sent, nbytes);

        seL4_Call(SYSCALL_ENDPOINT_SLOT, tag);

        /* Even if the bytes didnt get sent correctly, we dont resend them */
        bytes_remaining -= nbytes;
        bytes_sent += (size_t)seL4_GetMR(0); /* Receive back number of bytes sent */
    }

    return bytes_sent;
}

size_t sos_read(void *data, size_t count) {
    //implement this to use your syscall
    return 0;
}

