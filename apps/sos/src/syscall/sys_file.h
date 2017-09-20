/*
 * File Syscalls
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#ifndef _SYS_FILE_H_
#define _SYS_FILE_H_

#include <sel4/sel4.h>
#include <proc/proc.h>

/*
 * Syscall to open a file
 * msg(1) path_vaddr
 * msg(2) mode
 * @returns nwords in return message
 */
seL4_Word syscall_open(proc *curproc);

/*
 * Syscall to write to a file
 * msg(1) fd
 * msg(2) buf_vaddr
 * msg(3) buff_size
 * @returns nwords in return message
 */
seL4_Word syscall_write(proc *curproc);

/*
 * Syscall to read from a file
 * msg(1) fd
 * msg(2) buf_vaddr
 * msg(3) buff_size
 * @returns nwords in return message 
 */
seL4_Word syscall_read(proc *curproc);

/*
 * Syscall to close to a file
 * msg(1) fd
 * @returns nwords in return message
 */
seL4_Word syscall_close(proc *curproc);

/*
 * Syscall to stat to a file
 * msg(1) name
 * msg(2) buf_vaddr
 * @returns nwords in return message
 */
seL4_Word syscall_stat(proc *curproc);

/*
 * Syscall to stat to a file
 * msg(1) name
 * msg(2) buf_vaddr
 * @returns nwords in return message
 */
seL4_Word syscall_stat(proc *curproc);

/*
 * Syscall to list all files
 * msg(1) dir
 * msg(2) nfiles
 * @returns nwords in return message
 */
seL4_Word syscall_listdir(proc *curproc);

#endif /* _SYS_FILE_H_ */
