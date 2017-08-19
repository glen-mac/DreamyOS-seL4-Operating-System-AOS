/*
 * Process Management
 *
 * Cameron Lonsdale & Glenn McGuire
 */

#ifndef _PROC_H_
#define _PROC_H_

#include <sel4/sel4.h>
#include <cspace/cspace.h>
#include "addrspace.h"
#include "vm.h"

/* maximum number of processes to support */
#define MAX_PROCS 32

/* entry for a process control block */
typedef struct {
    seL4_Word tcb_addr;
    seL4_TCB tcb_cap;

    addrspace *p_addrspace;

    seL4_Word ipc_buffer_addr;  
    seL4_CPtr ipc_buffer_cap;

    cspace_t *croot;

} proc;

/* global process array */
proc sos_procs[MAX_PROCS];

/* hacky first proc */
#define tty_test_process (&sos_procs[0])
#define curproc (tty_test_process)

void start_first_process(char *_cpio_archive, char* app_name, seL4_CPtr fault_ep);

#endif /* _PROC_H_ */
