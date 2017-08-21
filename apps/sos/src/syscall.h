/*
 * Syscall handler
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <sel4/sel4.h>
#include <stdlib.h>

#include <serial/serial.h>

/* SERIAL PORT IS A HACK FOR NOW */

/*
 * Process a System Call
 * @param badge, the badge of the capability
 * @param nwords, the size of the message in words (not to be trusted)
 */
void handle_syscall(struct serial *serial_port, seL4_Word badge, size_t nwords);

#endif /* _SYSCALL_H_ */
