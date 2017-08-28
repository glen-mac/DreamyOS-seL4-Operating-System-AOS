/*
 * File Syscall handler
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include <serial/serial.h>

/*
 * Syscall to open a file
 * msg(1) path_vaddr
 * msg(2) mode
 */
void syscall_open(seL4_CPtr reply_cap);

/*
 * Syscall to write to a file
 * msg(1) fd
 * msg(2) buf_vaddr
 * msg(3) buff_size 
 */
void syscall_write(seL4_CPtr reply_cap);

/*
 * Syscall to read from a file
 * msg(1) fd
 * msg(2) buf_vaddr
 * msg(3) buff_size 
 */
void syscall_read(seL4_CPtr reply_cap);

/*
 * Syscall to close to a file
 * msg(1) fd
 */
void syscall_close(seL4_CPtr reply_cap);
