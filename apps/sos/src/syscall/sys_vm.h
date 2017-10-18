/*
 * VM Syscall handler
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#ifndef _SYS_VM_H_
#define _SYS_VM_H_

#include <proc/proc.h>

/* 
 * Syscall for setting the end of the heap
 * msg(1) newbrk
 * @returns nwords in return message
 */
int syscall_brk(proc *curproc);

#endif /* _SYS_VM_H_ */
