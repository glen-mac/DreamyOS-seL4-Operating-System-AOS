/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _CLOCK_H_
#define _CLOCK_H_

#include <stdint.h>
#include <sel4/sel4.h>

/* Physical address of the memory mapped timer */
#define CLOCK_GPT 0x2098000
#define CLOCK_GPT_NUM_REGS 10
#define CLOCK_GPT_SIZE 4 * CLOCK_GPT_NUM_REGS /* In bytes */

/*
 * Return codes for driver functions
 */
#define CLOCK_R_OK   (0)        /* success */
#define CLOCK_R_UINT (-1)       /* driver not initialised */
#define CLOCK_R_CNCL (-2)       /* operation cancelled (driver stopped) */
#define CLOCK_R_FAIL (-3)       /* operation failed for other reason */

typedef int64_t timestamp_t;
typedef void (*timer_callback_t)(uint32_t id, void *data);

/*
 * Initialise the timer.
 * @param mapped_vaddr, the address of the memory mapped registers for GPT
 */
void init_timer(void *vaddr);

/*
 * Start the timer.
 * Enable interrupt requests and set GPT registers.
 * @param interrupt_ep, badged async endpoint that driver uses for delivering interrupts
 * @returns CLOCK_R_OK iff successful
 */
int start_timer(seL4_CPtr interrupt_ep);

/*
 * Register a callback to be called after a given delay
 * @param uint64_t delay, microsecond delay before event
 * @param timer_callback_t callback, function to be run after delay
 * @param void *data, data to be passed to the callback function
 * @return id of the timer event
 */
uint32_t register_timer(uint64_t delay, timer_callback_t callback, void *data);

/*
 * Register a repeating callback to be run every delay microseconds
 * @param uint64_t delay, microsecond delay before event
 * @param timer_callback_t callback, function to be run after delay
 * @param void *data, data to be passed to the callback function
 * @return id of the timer event
 */
uint32_t register_repeating_timer(uint64_t delay, timer_callback_t callback, void *data);

/*
 * Remove a previously registered callback by its ID
 * @param uint32_t id of the registered timer
 * @return CLOCK_R_OK iff successful.
 */
int remove_timer(uint32_t id);

/*
 * Handle an interrupt message sent to 'interrupt_ep' from start_timer
 * @returns CLOCK_R_OK iff successful
 */
int timer_interrupt(void);

/*
 * Get the present time in microseconds since booting.
 * @returns 64 bit timestamp on success, else negative value
 */
timestamp_t time_stamp(void);

/*
 * Stop clock driver operation.
 * @returns CLOCK_R_OK iff successful.
 */
int stop_timer(void);

#endif /* _CLOCK_H_ */
