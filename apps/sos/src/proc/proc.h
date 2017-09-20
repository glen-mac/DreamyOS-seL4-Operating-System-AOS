/*
 * Process Management
 *
 * Cameron Lonsdale & Glenn McGuire
 */

#ifndef _PROC_H_
#define _PROC_H_

#include <sel4/sel4.h>
#include <cspace/cspace.h>

#include <vm/addrspace.h>
#include <vm/vm.h>
#include <vfs/file.h>

/* Process Struct */
typedef struct _proc {
    seL4_Word tcb_addr;
    seL4_TCB tcb_cap;

    seL4_Word ipc_buffer_addr;  
    seL4_CPtr ipc_buffer_cap;

    cspace_t *croot;

    addrspace *p_addrspace; /* Process address space */
    fdtable *file_table; /* Open files for a process */
    pid_t pid;
} proc;

pid_t proc_start(char *_cpio_archive, char *app_name, seL4_CPtr fault_ep);

/* 
 * Get proc structure given pid
 * @param pid, the pid of the process
 * @returns process struct pointer
 */
proc *get_proc(pid_t pid);

#endif /* _PROC_H_ */
