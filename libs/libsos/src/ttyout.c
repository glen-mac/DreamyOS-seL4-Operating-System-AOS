/*
 * tty console IO
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include "ttyout.h"

#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <sos.h>

#include <sel4/sel4.h>

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

        seL4_Call(SOS_IPC_EP_CAP, tag);

        /* Even if the bytes didnt get sent correctly, we dont resend them */
        bytes_remaining -= nbytes;
        bytes_sent += (size_t)seL4_GetMR(0); /* Receive back number of bytes sent */
    }

    return bytes_sent;
}
