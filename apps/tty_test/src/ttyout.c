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
    //implement this to use your syscall
    // Why does this cause a vm_fault? `printf("DEBUG: in sos_write\n"); 
    char *message = "Debug: in sos_write\n";
    sos_debug_print(message, strlen(message));

    seL4_MessageInfo_t tag;
    int num_arguments = 2;

    char *string = vData;

    for (int i = 0; i < count; i++) {
        /* Syscall 1 SOS_WRITE for every character*/
        tag = seL4_MessageInfo_new(0, 0, 0, num_arguments);
        seL4_SetTag(tag);
        seL4_SetMR(0, 1);
        seL4_SetMR(1, string[i]);
        seL4_Call(SYSCALL_ENDPOINT_SLOT, tag);
    }

    char *message2 = "Debug: after call\n";
    sos_debug_print(message2, strlen(message2));

    return 0;
}

size_t sos_read(void *vData, size_t count) {
    //implement this to use your syscall
    return 0;
}

