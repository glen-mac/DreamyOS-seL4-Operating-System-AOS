/*
 * Event loop
 *
 * Cameron Lonsdale & Glenn McGuire
 */

#ifndef _EVENT_H_
#define _EVENT_H_

#include <sel4/sel4.h>
#include <utils/arith.h>

/* 
 * To differencient between async and and sync IPC, we assign a
 * badge to the async endpoint. The badge that we receive will
 * be the bitwise 'OR' of the async endpoint badge and the badges
 * of all pending notifications.
 */
#define IRQ_EP_BADGE BIT(seL4_BadgeBits - 1)

/* 
 * All badged IRQs set high bet, then we use uniq bits to
 * distinguish interrupt sources
 */
#define IRQ_BADGE_NETWORK BIT(0)
#define IRQ_BADGE_TIMER BIT(1)

/* Endpoint capabilities */
extern seL4_CPtr _sos_ipc_ep_cap;
extern seL4_CPtr _sos_interrupt_ep_cap;

/*
 * Event handler loop
 * @param ep, the endpoint where all messages come in
 */
void event_loop(const seL4_CPtr ep);

#endif /*_EVENT_H_ */
