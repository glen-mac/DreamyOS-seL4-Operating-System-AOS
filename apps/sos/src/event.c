/*
 * Event loop
 *
 * Cameron Lonsdale & Glenn McGuire
 */

#include "event.h"

#include <clock/clock.h>
#include <syscall/syscall.h>
#include <proc/proc.h>
#include <utils/util.h>
#include "network.h"

/* Return the proc ID from the badge upper bits */
#define GET_PROCID_BADGE(x) (x >> 21)

/* Private functions */
static void start_first_proc(void);
static void reap_dead_orphans(proc *init);

void
event_loop(const seL4_CPtr ep)
{
    seL4_Word badge;
    seL4_Word label;
    seL4_MessageInfo_t message;

    /* Start the startup user application */
    resume(coroutine((void * (*)(void *))start_first_proc), NULL);

    proc *init = get_proc(0);
    assert(init != NULL);

    while (TRUE) {
        /* Cleanup any of SOS' children */
        reap_dead_orphans(init);

        message = seL4_Wait(ep, &badge);
        label = seL4_MessageInfo_get_label(message);
        /* Interrupt */
        if (badge & IRQ_EP_BADGE) {
            if (badge & IRQ_BADGE_NETWORK)
                network_irq();

            /* Needs to be an if, interrupts can group together */
            if (badge & IRQ_BADGE_TIMER)
                timer_interrupt();

        } else if (label == seL4_VMFault) {
            /* VM Fault */
            resume(coroutine((void * (*)(void *))vm_fault), (void *)GET_PROCID_BADGE(badge));
        } else if (label == seL4_NoFault) {
            /* Handle syscall */
            resume(coroutine((void * (*)(void *))handle_syscall), (void *)GET_PROCID_BADGE(badge));
        } else {
            LOG_INFO("Rootserver got an unknown message");
        }
    }
}

/*
 * Destroy a zombie child of init process
 * @param init, the init process
 */
static void 
reap_dead_orphans(proc *init)
{
    /* Search for a zombie child */
    for (struct list_node *curr = init->children->head; curr != NULL; curr = curr->next) {
        proc *child = get_proc((pid_t)curr->data);
        assert(child != NULL);
        if (child->p_state == ZOMBIE) {
            proc_destroy(child);
            /* 
             * We break because we dont want to keep iterating
             * as proc_destroy modifies the list, so if we keep iterating
             * we get invalid data.
             * Breaking is fine, we will cleanup other children the next time this is called
             */
            break;
        }
    }
}

/*
 * Start the first process 
 * Needs to be here because the elf file is read from NFS
 * Also use this function because the proc_start we want to call takes multiple arguments
 * which picoro doesnt support
 */
static void
start_first_proc(void)
{
    assert(proc_start(CONFIG_SOS_STARTUP_APP, _sos_ipc_ep_cap, 0) != -1);
}
