/*
 * Process Syscalls
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include "sys_proc.h"

#include "event.h" /* Only for CPIO archive and _sos_ipc_ep_cap */

#include <proc/proc.h>
#include <sys/panic.h>
#include <utils/util.h>


seL4_Word
syscall_proc_create(void)
{
    // TODO: Hard copy the filename because it might cross a page boundary
    seL4_Word name = vaddr_to_sos_vaddr(seL4_GetMR(1), ACCESS_READ);

    pid_t pid = proc_start(_cpio_archive, name, _sos_ipc_ep_cap);
    if (pid == -1)
        LOG_ERROR("Error starting process");

    seL4_SetMR(0, (seL4_Word)pid);
    return 1; /* nwords in message */
}

seL4_Word
syscall_proc_delete(void)
{
    panic("not implemented");
}

seL4_Word
syscall_proc_id(void)
{
    panic("not implemented");
}

seL4_Word
syscall_proc_status(void)
{
    panic("not implemented");
}

seL4_Word
syscall_proc_wait(void)
{
    panic("not implemented");
}
