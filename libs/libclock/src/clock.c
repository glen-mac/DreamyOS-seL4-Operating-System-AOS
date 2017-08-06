/*
 * Clock Implementation
 * Cameron Lonsdale & Glenn McGuire
 *
 */

#include <clock/clock.h>

#include <clock/pq.h>
#include <cspace/cspace.h>

#include <stdio.h> // DEBUG

/* Interrupt Request ID */
#define GPT_IRQ 87

/* GPT Register offsets */
/* All registers 32 bits long */
#define GPT_CR 0x0 /* Control Register */
#define GPT_PR 0x4 /* Prescaler Register */
#define GPT_SR 0x8 /* Status Register */
#define GPT_IR 0xC /* Interrupt register */
#define GPT_OCR1 0x10 /* Output Compare Register 1 */
#define GPT_CNT 0x24 /* Counter Register */

#define PERIPHERAL_FREQUENCY 66; /* 66 Mhz */

/* Global var for the start of GPT */
void *gpt_virtual = NULL;

/* Global vars for the GPT timer registers */
seL4_Word *control_register_ptr;
seL4_Word *prescale_register_ptr;
seL4_Word *interrupt_register_ptr;
seL4_Word *status_register_ptr;
seL4_Word *compare_register_ptr;
seL4_Word *counter_register_ptr;

seL4_CPtr irq_handler; /* Global IRQ handler */
priority_queue *pq; /* timer event pq handler */

int timer_is_initialised(void);


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

    control_register_ptr = (seL4_Word *)(gpt_virtual + GPT_CR);
    prescale_register_ptr = (seL4_Word *)(gpt_virtual + GPT_PR);
    interrupt_register_ptr = (seL4_Word *)(gpt_virtual + GPT_IR);
    status_register_ptr = (seL4_Word *)(gpt_virtual + GPT_SR);
    compare_register_ptr = (seL4_Word *)(gpt_virtual + GPT_OCR1);
    counter_register_ptr = (seL4_Word *)(gpt_virtual + GPT_CNT);

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
    /* Stop timer if initialised */
    if (timer_is_initialised())
        stop_timer();

    /* Set timer interupts to be sent to interrupt_ep, and creates an interrupt capability */
    if (enable_irq(GPT_IRQ, interrupt_ep, &irq_handler) != 0)
        return CLOCK_R_FAIL; /* Operation failed for other reason (IRQ failed setup) */

    /* Reset key registers */
    *control_register_ptr = 0x00000000;
    *prescale_register_ptr = 0x00000000;
    *status_register_ptr = 0x00000000;
    *interrupt_register_ptr = 0x00000000;
    *compare_register_ptr = 0xFFFFFFFF;

    /* Set the timer registers for settings */
    *control_register_ptr |= 1 << 9; /* Free-Run mode */
    *control_register_ptr |= 1 << 6; /* peripheral clock */
    *control_register_ptr |= 1 << 5; /* stop mode */
    *control_register_ptr |= 1 << 4; /* doze mode */
    *control_register_ptr |= 1 << 3; /* wait mode */
    *control_register_ptr |= 1 << 2; /* debug mode */
    *control_register_ptr &= ~(1 << 1); /*  ENMOD retain value */

    *interrupt_register_ptr |= 1 << 5; /* Roll over enabled */
    *interrupt_register_ptr |= 1 << 0; /* Output compare channel 1 enabled */

    /* Since clock is 66Mhz, prescale of 66 means the counter increases by 1 every 1/1,000,000 a second */
    /* Which is equivalent to counting microseconds */
    *prescale_register_ptr = PERIPHERAL_FREQUENCY;

    /* Start the clock */
    *control_register_ptr |= 1 << 0;

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
    if (!timer_is_initialised())
        return CLOCK_R_OK; /* Return 0 on failure */

    uint64_t time = time_stamp() + delay;
    //printf("time: %lld\n", time);    
    int id = pq_push(pq, time, callback, data);

    // more  hack
    time = pq_time_peek(pq);
    //printf("time: %lld\n", time);
    *compare_register_ptr = time;

    return id;
}

/*
 * Remove a previously registered callback by its ID
 *    id: Unique ID returned by register_time
 * Returns CLOCK_R_OK iff successful.
 */
int remove_timer(uint32_t id) {
    if (!timer_is_initialised())
        return CLOCK_R_UINT; /* Driver not initialised */

    if (!pq_remove(pq, id))
        return CLOCK_R_FAIL; /* Operation failed for other reason (invalid id) */

    /* The timer we just removed could have been the next event so we need to update the compare reg */
    *compare_register_ptr = pq_time_peek(pq);

    return CLOCK_R_OK;
}

/*
 * Handle an interrupt message sent to 'interrupt_ep' from start_timer
 *
 * Returns CLOCK_R_OK iff successful
 */
int timer_interrupt(void) {
    // TODO: Deal with counter register overflow, we can check the status register to see if that interrupt has occured

    /* run the callback event */
    do {
        event *curEvent = pq_pop(pq);
        curEvent->callback(curEvent->uid, curEvent->data);
    } while (!pq_is_empty(pq) && pq_time_peek(pq) <= time_stamp() + 1000); /* 1ms buffer */
    // TOOD: IM NOT SURE THAT A 1ms BUFFER IS A GOOD IDEA

    if (pq_is_empty(pq)) {
        // TODO: Is turning off the compare register interrupt a better idea? 
        // And then we can turn it back on in register_timer when pq_is_empty?
        *compare_register_ptr = 0xFFFFFFFF;
    } else {
        /* Set the next compare value to the event at the front of the priority queue */
        *compare_register_ptr = pq_time_peek(pq);
    }

    // TODO: THIS WONT ACKNOWLEDGE A ROLL OVER EVENT

    *status_register_ptr |= 1 << 0; /* Acknowledge a compare event occured */

    /* Acknowledge the interrupt so more can happen */
    if (seL4_IRQHandler_Ack(irq_handler) != 0)
        return CLOCK_R_FAIL; /* Operation failed for other reason (Failed to ack interrupt) */

    return CLOCK_R_OK;
}

/*
 * Returns present time in microseconds since booting.
 *
 * Returns a negative value if failure.
 */
timestamp_t time_stamp(void) {
    if (!timer_is_initialised())
        return CLOCK_R_UINT; /* Driver not initialised */

    // TODO: DO 64 BIT TIMESTAMPS

    return (timestamp_t)(*counter_register_ptr);
}

/*
 * Stop clock driver operation.
 *
 * Returns CLOCK_R_OK iff successful.
 */
int stop_timer(void) {
    // TODO: Turn off receiving interrupts

    // TODO: Turn off the timer

    // TODO: Remove all events from the priority queue

    return CLOCK_R_UINT;
}


/* Checks if the timer has been initialised, If gpt_virtual is NULL then init_timer hasnt been called */
int timer_is_initialised(void) {
    return gpt_virtual == NULL? 0: 1;
}
