/*
 * Clock Implementation
 * Cameron Lonsdale & Glenn McGuire
 *
 */

#include <clock/clock.h>

#include <stdio.h>
#include <cspace/cspace.h>

#include <clock/pq.h>

/* irq ids */
#define GPT_IRQ 87
#define EPIT1_IRQ 88
#define EPIT2_IRQ 89

/* EPIT1 Register offsets */
#define EPIT1_CONTROLREG 0 /* Control register; 32 bits */
#define EPIT1_STATUSREG 4 /* Status register; 32 bits */
#define EPIT1_LOADREG 8 /* Load register; 32 bits */
#define EPIT1_COMPAREREG 12 /* Compare register; 32 bits */
#define EPIT1_COUNTERREG 16 /* Counter Register; 32 bits */

/* GPT Register offsets */
#define GPT_CONTROLREG 0x0 /* Control register; 32 bits */
#define GPT_PRESCALEREG 0x4 /* Status register; 32 bits */
#define GPT_STATUSREG 0x8 /* Load register; 32 bits */
#define GPT_INTERRUPTREG 0xC /* Compare register; 32 bits */
#define GPT_COMPARE1REG 0x10 /* Compare register; 32 bits */
#define GPT_CNT 0x24 /* Compare register; 32 bits */

void *gpt_virtual; /* Global var for the start of EPIT */
seL4_CPtr irq_handler; /* Global IRQ handler */
priority_queue *pq; /* timer event pq handler */

/* This is copied from network.c (and modified), we should abstract this out into a "interrupt.h" or something */
int enable_irq(int irq, seL4_CPtr aep, seL4_CPtr *irq_handler_ptr) {
    /* Create an IRQ handler */
    if (!(*irq_handler_ptr = cspace_irq_control_get_cap(cur_cspace, seL4_CapIRQControl, irq)))
        return -1;
    
    /* Assign to an end point */
    if (seL4_IRQHandler_SetEndpoint(*irq_handler_ptr, aep) != 0)
        return -1;

    /* Ack the handler before continuing */
    if (seL4_IRQHandler_Ack(*irq_handler_ptr) != 0)
        return -1;

    return 0;
}

/* Initialise the GPT handler */
void init_timer(void *vaddr) {
    gpt_virtual = vaddr;
    pq = init_pq(); 
}

/*
 * Initialise driver. Performs implicit stop_timer() if already initialised.
 *    interrupt_ep:       A (possibly badged) async endpoint that the driver
                          should use for deliverying interrupts to
 *
 * Returns CLOCK_R_OK iff successful.
 */
int start_timer(seL4_CPtr interrupt_ep) {

    /* Stop timer, doesn't matter if it hasn't already started */
    stop_timer();

    /* Set the timer registers for settings */

    /* Map the device frame into virtual memory */
    seL4_Word *control_register_ptr = (seL4_Word *)(gpt_virtual + GPT_CONTROLREG);
    seL4_Word *prescale_register_ptr = (seL4_Word *)(gpt_virtual + GPT_PRESCALEREG);
    seL4_Word *interrupt_register_ptr = (seL4_Word *)(gpt_virtual + GPT_INTERRUPTREG);
    seL4_Word *status_register_ptr = (seL4_Word *)(gpt_virtual + GPT_STATUSREG);
    seL4_Word *compare_register_ptr = (seL4_Word *)(gpt_virtual + GPT_COMPARE1REG);

    /* Set timer interupts to be sent to interrupt_ep, and creates an interrupt capability */
    if (enable_irq(GPT_IRQ, interrupt_ep, &irq_handler) != 0)
        return CLOCK_R_UINT;

    /* Reset the registers which we do not completely overwrite*/
    *control_register_ptr = 0x00000000;
    *prescale_register_ptr = 0x00000000;
    *status_register_ptr = 0x00000000;
    *interrupt_register_ptr = 0x00000000;

    *control_register_ptr |= 1 << 9; /* Free-Run mode */
    *control_register_ptr |= 1 << 6; /* peripheral clock */
    *control_register_ptr |= 1 << 5; /* stop mode */
    *control_register_ptr |= 1 << 4; /* doze mode */
    *control_register_ptr |= 1 << 3; /* wait mode */
    *control_register_ptr |= 1 << 2; /* debug mode */
    *control_register_ptr &= ~(1 << 1); /*  ENMOD retain value */

    *interrupt_register_ptr |= 1 << 5; /* Roll over enabled */
    *interrupt_register_ptr |= 1 << 0; /* Output compare channel 1 enabled */

    *prescale_register_ptr = 66;

    *compare_register_ptr = 1000000; /* 1 million microseconds value */

    *control_register_ptr |= 1 << 0; /* clock started */

    return CLOCK_R_OK;
}

/*
 * Register a callback to be called after a given delay
 *    delay:  Delay time in microseconds before callback is invoked
 *    callback: Function to be called
 *    data: Custom data to be passed to callback function
 *
 * Returns 0 on failure, otherwise an unique ID for this timeout
 */
uint32_t register_timer(uint64_t delay, timer_callback_t callback, void *data) {
    return pq_push(pq, time_stamp()+delay, callback, data);
}

/*
 * Remove a previously registered callback by its ID
 *    id: Unique ID returned by register_time
 * Returns CLOCK_R_OK iff successful.
 */
int remove_timer(uint32_t id) {
    if (!pq_remove(pq, id))
        return CLOCK_R_UINT;
    return CLOCK_R_OK;
}

/*
 * Handle an interrupt message sent to 'interrupt_ep' from start_timer
 *
 * Returns CLOCK_R_OK iff successful
 */
int timer_interrupt(void) {
    /* TODO work out which call back to run? */

    // TODO: Deal with counter register overflow
    seL4_Word *status_register_ptr = (seL4_Word *)(gpt_virtual + GPT_STATUSREG);
    *status_register_ptr |= 1 << 0; /* Acknowledge a compare event occured */

    /* Acknowledge the interrupt so more can happen */
    if (seL4_IRQHandler_Ack(irq_handler) != 0)
        return CLOCK_R_UINT;

    /* SET THE NEXT COMPARE VALUE GIVEN THE FRONT OF THE PQ */
    (void) pq_time_peek(pq);

    /* run the callback event */
    event *curEvent = pq_pop(pq);
    curEvent->callback(curEvent->uid, curEvent->data);

    return CLOCK_R_OK;
}

/*
 * Returns present time in microseconds since booting.
 *
 * Returns a negative value if failure.
 */
timestamp_t time_stamp(void) {
    // TODO: What would fail here? Driver not initialised I guess.

    seL4_Word *counter_register = (seL4_Word *)(gpt_virtual + GPT_CNT);
    return (timestamp_t)(*counter_register);
}

/*
 * Stop clock driver operation.
 *
 * Returns CLOCK_R_OK iff successful.
 */
int stop_timer(void) {
    return CLOCK_R_UINT;
}
