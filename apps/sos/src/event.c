/*
 * Event loop
 *
 * Cameron Lonsdale & Glenn McGuire
 */

#include "event.h"

#include "picoro.h"
#include <vm/vm.h>
#include <clock/clock.h>
#include <cspace/cspace.h>
#include <sys/debug.h>
#include <sys/panic.h>
#include <syscall/syscall.h>
#include <utils/util.h>
#include "network.h"

/* 
 * Syscall coroutine
 */
coro syscall_coro = NULL;

void
event_loop(const seL4_CPtr ep)
{
    seL4_Word badge;
    seL4_Word label;
    seL4_MessageInfo_t message;

    while (TRUE) {
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
            // THIS IS BAD, NEED TO FIX THIS ASAP
            syscall_coro = coroutine(vm_fault);
            resume(syscall_coro, NULL);
        } else if (label == seL4_NoFault) {
            syscall_coro = coroutine(handle_syscall);
            resume(syscall_coro, badge);
        } else {
            LOG_INFO("Rootserver got an unknown message");
        }
    }
}
