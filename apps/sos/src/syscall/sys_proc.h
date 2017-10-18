/*
 * Process Syscalls
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#ifndef _SYS_PROC_H_
#define _SYS_PROC_H_

#include <proc/proc.h>

/*
 * Syscall to create process
 * msg(1) name
 * @returns nwords in return message
 */
int syscall_proc_create(proc *curproc);

/*
 * Syscall to delete a process
 * msg(1) pid
 * @returns nwords in return message
 */
int syscall_proc_delete(proc *curproc);

/*
 * Syscall to return current proc pid
 * @returns nwords in return message 
 */
int syscall_proc_id(proc *curproc);

/*
 * Syscall to return status of current procs
 * msg(1) process_vaddr
 * msg(2) procs_max
 * @returns nwords in return message
 */
int syscall_proc_status(proc *curproc);

/*
 * Syscall to wait for a proc to exit
 * msg(1) pid
 * @returns nwords in return message
 */
int syscall_proc_wait(proc *curproc);

/*
 * Syscall to exit a process
 * Called when a process calls exit()
 * Process terminates after this call, but the process struct is still intact
 */
int syscall_exit(proc *curproc);

#endif /* _SYS_PROC_H_ */
