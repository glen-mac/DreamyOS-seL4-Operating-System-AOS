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

/* Hacky first proc */
// #define tty_test_process (sos_procs[0])
// #define curproc (tty_test_process)

/* Process Struct */
typedef struct {
    seL4_Word tcb_addr;
    seL4_TCB tcb_cap;

    seL4_Word ipc_buffer_addr;  
    seL4_CPtr ipc_buffer_cap;

    cspace_t *croot;

    addrspace *p_addrspace; /* Process address space */
    fdtable *file_table; /* Open files for a process */
    pid_t pid;
} proc;

/* start a proc and return the pid */
pid_t proc_start(char *_cpio_archive, char *app_name, seL4_CPtr fault_ep);

/* get ptr of proc struct given the pid */
proc *get_proc(pid_t pid);

#endif /* _PROC_H_ */
