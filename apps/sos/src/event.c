/*
 * Event loop
 *
 * Cameron Lonsdale & Glenn McGuire
 */

#include "event.h"

#include <vm/vm.h>
#include <clock/clock.h>
#include <cspace/cspace.h>
#include <sys/debug.h>
#include <sys/panic.h>
#include <syscall/syscall.h>
#include <proc/proc.h>
#include <utils/util.h>
#include "network.h"

void
event_loop(const seL4_CPtr ep)
{
    seL4_Word badge;
    seL4_Word label;
    seL4_MessageInfo_t message;

    while (TRUE) {
        proc *init = get_proc(0);
        for (struct list_node *curr = init->children->head; curr != NULL; curr = curr->next)
            LOG_INFO("init child: %d", curr->data);

        message = seL4_Wait(ep, &badge);
        label = seL4_MessageInfo_get_label(message);
        if (badge & IRQ_EP_BADGE) {
            /* Interrupt */
            if (badge & IRQ_BADGE_NETWORK)
                network_irq();

            /* Needs to be an if, interrupts can group together */
            if (badge & IRQ_BADGE_TIMER)
                timer_interrupt();

        } else if (label == seL4_VMFault) {
            resume(coroutine(vm_fault), GET_PROCID_BADGE(badge));
        } else if (label == seL4_NoFault) {
            resume(coroutine(handle_syscall), GET_PROCID_BADGE(badge));
        } else {
            LOG_INFO("Rootserver got an unknown message");
        }
    }
}
