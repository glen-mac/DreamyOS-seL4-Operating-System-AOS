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
    // Why does this cause a vm_fault?
    // printf("DEBUG: in sos_write\n"); 

    seL4_MessageInfo_t tag;
    char *message = vData;

    size_t buffer_size = seL4_MsgMaxLength; /* Measured in bytes */

    // Debug
    // char str[80];
    // sprintf(str, "count: %d\n", count);
    // sos_debug_print(str, 80);

    // sprintf(str, "buffersize: %d\n", buffer_size);
    // sos_debug_print(str, 80);

    // sprintf(str, "n_messages: %d\n", (count / buffer_size) + 1);
    // sos_debug_print(str, 80);

    // sprintf(str, "nwords: %d\n", (count + sizeof(seL4_Word) - 1) / sizeof(seL4_Word));
    // sos_debug_print(str, 80);

    size_t bytes_left = count;
    size_t bytes_sent = 0;

    /*
     * Because count can be larger than message buffer size,
     * we need to break up the message into several messages.
     */
    for (size_t message_id = 0; message_id < (count / buffer_size) + 1; ++message_id) {
        size_t nbytes = bytes_left > buffer_size ? buffer_size: bytes_left;

        /* 2 comes from the syscall and nbytes fields */
        size_t nwords = 2 + (nbytes + sizeof(seL4_Word) - 1) / sizeof(seL4_Word);
        tag = seL4_MessageInfo_new(0, 0, 0, nwords);
        seL4_SetTag(tag);
        seL4_SetMR(0, 1); /* Syscall number */
        seL4_SetMR(1, nbytes); /* Number of bytes in the message */

        memcpy(seL4_GetIPCBuffer()->msg + 2, message + bytes_sent, nbytes);

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

