/*
 * Event loop
 *
 * Cameron Lonsdale & Glenn McGuire
 */

#ifndef _EVENT_H_
#define _EVENT_H_

#include <sel4/sel4.h>

/* 
 * To differencient between async and and sync IPC, we assign a
 * badge to the async endpoint. The badge that we receive will
 * be the bitwise 'OR' of the async endpoint badge and the badges
 * of all pending notifications.
 */
#define IRQ_EP_BADGE (1 << 8)

/* return the proc ID from the badge upper bits */
#define GET_PROCID_BADGE(x) (x >> 27)

/* return the badge with the proc set in upper bits */
#define SET_PROCID_BADGE(b, pid) ((b & 0x7FFFFFF) | (pid << 27))

/* 
 * All badged IRQs set high bet, then we use uniq bits to
 * distinguish interrupt sources
 */
#define IRQ_BADGE_NETWORK (1 << 0)
#define IRQ_BADGE_TIMER (1 << 1)

extern seL4_CPtr _sos_ipc_ep_cap;
extern seL4_CPtr _sos_interrupt_ep_cap;

/*
 * The linker will link this symbol to the start address
 * of an archive of attached applications.
 */
extern char _cpio_archive[];

/*
 * Event handler loop
 * @param ep, the endpoint where messages come in
 */
void event_loop(const seL4_CPtr ep);

#endif /*_EVENT_H_ */
