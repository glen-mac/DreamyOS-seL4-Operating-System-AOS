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
#include <stdio.h> // For debugging

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

size_t sos_write(void *vData, size_t count) {
    // Why does this cause a vm_fault? printf("DEBUG: in sos_write\n"); 

    seL4_MessageInfo_t tag;
    char *message = vData;
    size_t buffer_size = 1 << 6; /* Measured in words or is it bytes now? confusing */

    /* Because count can be larger than message buffer size,
     * we need to break up the message into several messages.
     */
    char str[80];
    sprintf(str, "count: %d\n", count);
    sos_debug_print(str, 80);

    sprintf(str, "buffersize: %d\n", buffer_size);
    sos_debug_print(str, 80);

    sprintf(str, "n_messages: %d\n", (count / buffer_size) + 1);
    sos_debug_print(str, 80);

    size_t bytes_left = count;
    size_t bytes_sent = 0;

    for (size_t message_id = 0; message_id < (count / buffer_size) + 1; ++message_id) {
        size_t nbytes = bytes_left > buffer_size ? buffer_size: bytes_left;

        tag = seL4_MessageInfo_new(0, 0, 0, 1 + nbytes);
        seL4_SetTag(tag);
        seL4_SetMR(0, 1); // Syscall number

        memcpy(seL4_GetIPCBuffer()->msg + sizeof(seL4_Word), message + bytes_sent, nbytes);

        seL4_Call(SYSCALL_ENDPOINT_SLOT, tag);

        bytes_left -= nbytes;
        bytes_sent += (size_t)seL4_GetMR(0); /* Receive back number of bytes sent */
    }

    return bytes_sent;
}

size_t sos_read(void *vData, size_t count) {
    //implement this to use your syscall
    return 0;
}

