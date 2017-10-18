/*
 * Process Management
 *
 * Cameron Lonsdale & Glenn McGuire
 */

#ifndef _PROC_H_
#define _PROC_H_

#include <coro/picoro.h>
#include <cspace/cspace.h>
#include <sel4/sel4.h>
#include <utils/list.h>
#include <vfs/file.h>
#include <vm/addrspace.h>
#include <vm/vm.h>

/* Maximum number of processes to support */
#define MAX_PROCS 32

/* process states enum */
typedef enum {
    CREATED, /* Created but not started */
    ZOMBIE,  /* Exited but not reclaimed by parent */
    RUNNING, /* Running uninterrupted */
    BLOCKED  /* Blocked on SOS */
} proc_states;

/* Process Struct */
typedef struct _proc {
    seL4_Word tcb_addr;             /* Physical address of the TCB */
    seL4_TCB tcb_cap;               /* TCB Capability */
    seL4_CPtr ipc_buffer_cap;       /* IPC buffer cap */
    cspace_t *croot;                /* cspace root pointer */

    addrspace *p_addrspace;         /* Process address space */
    fdtable *file_table;            /* File table */
    list_t *children;               /* Linked list of children */

    pid_t waiting_on;               /* Pid of the child proc is waiting on */
    coro waiting_coro;              /* Coroutine to resume when the wait is satisfied */

    pid_t ppid;                     /* Parent pid */
    pid_t pid;                      /* Pid of process */
    char *proc_name;                /* Process name */
    int64_t stime;                  /* Process start time */
    proc_states p_state;            /* Enum of the process state */

    size_t blocked_ref;             /* Number of blocks on this process */
    bool kill_flag;                 /* Flag to specify if this process has received a kill signal */
    bool protected;                 /* Flag to specify if the process can be killed */
} proc;

/*
 * Bootstrap the init process
 * @returns pid of the init process (should be 0)
 */
pid_t proc_bootstrap(void);

/*
 * Start a process
 * @param app_name, executible name
 * @param fault_ep, endpoint for IPC
 * @param parent_pid, the pid of the parent process for this new child
 * @return -1 on error, pid on success
 */
pid_t proc_start(char *app_name, seL4_CPtr fault_ep, pid_t parent_pid);

/*
 * Delete a process
 * Remove all data required for the process to run
 * @param victim, the process to delete
 * @returns 0 on success, else 1
 */
int proc_delete(proc *victim);

/*
 * Destroy a process
 * @param current, the process to destroy
 */
void proc_destroy(proc *current);

/*
 * Check if pid is a child of curproc
 * @param curproc, parent process
 * @param pid, the pid of the process to test
 * @return TRUE if pid is a child of curproc, else FALSE
 */
bool proc_is_child(proc *curproc, pid_t pid);

/* 
 * Get proc structure given pid
 * @param pid, the pid of the process
 * @returns process struct pointer on success, else NULL
 */
proc *get_proc(pid_t pid);

/*
 * Change proc state
 * @param pid, the pid of the process to mark
 * @param state, the new state of the proc
 */
void proc_mark(proc *curproc, proc_states state);

/*
 * Reparent children from curproc to pid
 * @param cur_parent, the current parent
 * @param new_parent, pid of the new parent
 * @returns 0 on success, else 1
 */
int proc_reparent_children(proc *cur_parent, pid_t new_parent);

#endif /* _PROC_H_ */
