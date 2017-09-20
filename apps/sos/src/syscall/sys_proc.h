/*
 * Process Syscalls
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#ifndef _SYS_PROC_H_
#define _SYS_PROC_H_

#include <sel4/sel4.h>
#include <proc/proc.h>

/*
 * Syscall to create process
 * msg(1) path_vaddr
 * msg(2) path_len
 * @returns nwords in return message
 */
seL4_Word syscall_proc_create(proc *curproc);

/*
 * Syscall to delete a process
 * msg(1) pid
 * @returns nwords in return message
 */
seL4_Word syscall_proc_delete(proc *curproc);

/*
 * Syscall to return current proc pid
 * @returns nwords in return message 
 */
seL4_Word syscall_proc_id(proc *curproc);

/*
 * Syscall to return status of current procs
 * msg(1) processes_vaddr
 * msg(2) max
 * @returns nwords in return message
 */
seL4_Word syscall_proc_status(proc *curproc);

/*
 * Syscall to wait for a proc to exit
 * msg(1) pid
 * @returns nwords in return message
 */
seL4_Word syscall_proc_wait(proc *curproc);

#endif /* _SYS_PRC_H_ */
