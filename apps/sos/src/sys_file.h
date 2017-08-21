/*
 * File Syscall handler
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include <serial/serial.h>

/* syscall for write */
void syscall_write(seL4_CPtr, void *, struct serial *);
