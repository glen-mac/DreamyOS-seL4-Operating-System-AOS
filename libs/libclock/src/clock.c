/*
 * Clock Implementation
 * Cameron Lonsdale & Glenn McGuire
 *
 */

#include <clock/clock.h>

#include <stdio.h>
#include <cspace/cspace.h>

#include <utils/pq.h>

#define GPT_IRQ 87
#define EPIT1_IRQ 88
#define EPIT2_IRQ 89

/* EPIT1 Register locations */
#define EPIT1_CR 0x20D0000 /* Control register; 32 bits */
#define EPIT1_SR 0x20D0004 /* Status register; 32 bits */
#define EPIT1_LR 0x20D0008 /* Load register; 32 bits */
#define EPIT1_CMPR 0x20D000C /* Compare register; 32 bits */
#define EPIT1_CNR 0x20D0010 /* Counter Register; 32 bits */

/* EPIT2 Register locations */
#define EPIT2_CR 0x20D4000 /* Control register; 32 bits */
#define EPIT2_SR 0x20D4004 /* Status register; 32 bits */
#define EPIT2_LR 0x20D4008 /* Load register; 32 bits */
#define EPIT2_CMPR 0x20D400C /* Compare register; 32 bits */
#define EPIT2_CNR 0x20D4010 /* Counter Register; 32 bits */

void *epit1_virtual; /* Global var for the start of EPIT */

/* DEBUG */
#define dprintf(...) do { \
        printf("%s %s %d: ", __FILE__, __func__, __LINE__); \
        printf(__VA_ARGS__); \
    }while(0)


/* This is copied from network.c (and modified), we should abstract this out into a "interrupt.h" or something */
int enable_irq(int irq, seL4_CPtr aep, seL4_CPtr *irq_handler) {
    /* Create an IRQ handler */
    if (!(*irq_handler = cspace_irq_control_get_cap(cur_cspace, seL4_CapIRQControl, irq)))
        return -1;
    
    /* Assign to an end point */
    if (seL4_IRQHandler_SetEndpoint(*irq_handler, aep) != 0)
        return -1;

    /* Ack the handler before continuing */
    if (seL4_IRQHandler_Ack(*irq_handler) != 0)
        return -1;

    return 0;
}

void init_timer(void *vaddr) {
    epit1_virtual = vaddr;
}


/*
 * Initialise driver. Performs implicit stop_timer() if already initialised.
 *    interrupt_ep:       A (possibly badged) async endpoint that the driver
                          should use for deliverying interrupts to
 *
 * Returns CLOCK_R_OK iff successful.
 */
int start_timer(seL4_CPtr interrupt_ep) {

    /* Stop timer, doesnt matter if it hasnt already started */
    stop_timer();

    /* Set the timer registers for settings */

    /* Map the device frame into virtual memory */
    seL4_Word *control_register_ptr = (seL4_Word *)epit1_virtual;
    seL4_Word *load_register_ptr = (seL4_Word *)(epit1_virtual + 8);
    seL4_Word *compare_register_ptr = (seL4_Word *)(epit1_virtual + 12);


    /* Set timer interupts to be sent to interrupt_ep, and creates an interrupt capability */
    seL4_CPtr cap;
    if (enable_irq(EPIT1_IRQ, interrupt_ep, &cap) != 0)
        return CLOCK_R_UINT;

    *control_register_ptr |= 1 << 24; /* Use Peripheral clock */
    *control_register_ptr |= 1 << 2; /* Output compare interrupt enable */
    *control_register_ptr |= 1 << 1; /* EPIT Enable mode */
    *control_register_ptr |= 1 << 0; /* Enable EPIT */

    *load_register_ptr = 2;
    *compare_register_ptr = 2; /* The value to count down from */

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
    return 0;
}

/*
 * Remove a previously registered callback by its ID
 *    id: Unique ID returned by register_time
 * Returns CLOCK_R_OK iff successful.
 */
int remove_timer(uint32_t id) {
    return CLOCK_R_UINT;
}

/*
 * Handle an interrupt message sent to 'interrupt_ep' from start_timer
 *
 * Returns CLOCK_R_OK iff successful
 */
int timer_interrupt(void) {
    dprintf("Timer interrupt occured\n");

    /* TODO work out which call back to run? */

    return CLOCK_R_OK;
}

/*
 * Returns present time in microseconds since booting.
 *
 * Returns a negative value if failure.
 */
timestamp_t time_stamp(void) {
    return CLOCK_R_UINT;
}

/*
 * Stop clock driver operation.
 *
 * Returns CLOCK_R_OK iff successful.
 */
int stop_timer(void) {
    return CLOCK_R_UINT;
}
