/*
 * Process Syscalls
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#ifndef _SYS_PROC_H_
#define _SYS_PROC_H_

#include <sel4/sel4.h>

/*
 * Syscall to create process
 * msg(1) path_vaddr
 * msg(2) path_len
 * @returns nwords in return message
 */
seL4_Word syscall_proc_create(void);

/*
 * Syscall to delete a process
 * msg(1) pid
 * @returns nwords in return message
 */
seL4_Word syscall_proc_delete(void);

/*
 * Syscall to return current proc pid
 * @returns nwords in return message 
 */
seL4_Word syscall_proc_id(void);

/*
 * Syscall to return status of current procs
 * msg(1) processes_vaddr
 * msg(2) max
 * @returns nwords in return message
 */
seL4_Word syscall_proc_status(void);

/*
 * Syscall to wait for a proc to exit
 * msg(1) pid
 * @returns nwords in return message
 */
seL4_Word syscall_proc_wait(void);

#endif /* _SYS_PRC_H_ */
