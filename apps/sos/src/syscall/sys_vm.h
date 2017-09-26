/*
 * VM Syscall handler
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#ifndef _SYS_VM_H_
#define _SYS_VM_H_

#include <sel4/sel4.h>
#include <proc/proc.h>

/* 
 * Syscall for extend the heap
 * msg(1) newbrk
 * @returns nwords in return message
 */
int syscall_brk(proc * curproc);

#endif /* _SYS_VM_H_ */
