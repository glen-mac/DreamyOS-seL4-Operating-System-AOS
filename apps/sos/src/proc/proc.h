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

/* maximum saved proc name */
#define N_NAME 32

/* Maximum number of processes to support */
#define MAX_PROCS 32

/* process states enum */
enum process_states {
    ZOMBIE,
    RUNNING
} proc_states;

/* Process Struct */
typedef struct _proc {

    seL4_TCB tcb_cap;               /* thread control block cap */
    seL4_CPtr ipc_buffer_cap;       /* ipc buffer cap */

    cspace_t *croot;                /* the cspace root */

    addrspace *p_addrspace;         /* the address space */
    fdtable *file_table;            /* file table ptr */

    pid_t pid;                      /* pid of process */
    char proc_name[N_NAME];         /* process name */
    int64_t stime;                  /* process start time */
    proc_states p_state;            /* enum of the process state */

} proc;

pid_t proc_start(char *_cpio_archive, char *app_name, seL4_CPtr fault_ep);

/* 
 * Get proc structure given pid
 * @param pid, the pid of the process
 * @returns process struct pointer
 */
proc *get_proc(pid_t pid);

#endif /* _PROC_H_ */
