/*
 * Clock Implementation
 * Cameron Lonsdale & Glenn McGuire
 *
 */

#include <clock/clock.h>

#include <clock/pq.h>
#include <cspace/cspace.h>

/* Interrupt Request ID */
#define GPT_IRQ 87

/* GPT Register offsets */
/* All registers 32 bits long */
#define GPT_CR 0x0 /* Control Register */
#define GPT_PR 0x4 /* Prescaler Register */
#define GPT_SR 0x8 /* Status Register */
#define GPT_IR 0xC /* Interrupt register */
#define GPT_OCR1 0x10 /* Output Compare Register 1 */
#define GPT_OCR2 0x14 /* Output Compare Register 2 */
#define GPT_CNT 0x24 /* Counter Register */

#define GPT_PERIPHERAL_CLOCK_FREQUENCY 66; /* Mhz */

/* Register reset values */
#define ZERO_RESET_VALUE 0x00000000
#define MAX_INT_RESET_VALUE 0xFFFFFFFF

/* Bit masks for setting bits in registers */
#define FREE_RUN_MASK (1 << 9)
#define PERIPHERAL_CLOCK_MASK (1 << 6)
#define STOP_MODE_MASK (1 << 5)
#define DOZE_MODE_MASK (1 << 4)
#define WAIT_MODE_MASK (1 << 3)
#define DEBUG_MODE_MASK (1 << 2)
#define ENMOD_MODE_MASK (1 << 1)
#define ROLL_OVER_MASK (1 << 5)
#define OUTPUT_COMPARE_MASK (1 << 0)

/* Definitions if a timer event is single use or repeating */
#define REPEAT_EVENT 1
#define SINGLE_EVENT 0

/* Flags to specify if an event failed and if a UID needs to be generated */
#define EVENT_FAIL 0
#define GEN_UID 0

/* Global var for the start of GPT mapped memory */
void *gpt_virtual = NULL;

/* Global vars for the GPT timer registers */
seL4_Word *control_register_ptr;
seL4_Word *prescale_register_ptr;
seL4_Word *interrupt_register_ptr;
seL4_Word *status_register_ptr;
seL4_Word *compare_register_ptr;
seL4_Word *upper_timestamp_register_ptr;
seL4_Word *counter_register_ptr;

seL4_CPtr irq_handler; /* Global IRQ handler */
priority_queue *pq; /* timer event pq handler */
uint32_t timer_started = 0; /* Boolean to state if the timer has started or not */

static uint64_t join32to64(uint32_t upper, uint32_t lower);
static uint32_t add_event_to_pq(uint64_t delay, timer_callback_t callback, void *data, uint8_t repeat, uint32_t uid);

/* 
 * Enable Interrupts for a sepcific IRQ and endpoint
 * This is copied from network.c (and modified)
 * TODO: Abstract this out so network.c and clock.c uses the same code?
 *
 * @param int irq, interrupt request id
 * @param seL4_CPtr aep, asynchronous endpoint the interrupt message will be delivered to
 * @param[out] seL4_CPtr *irq_handler_ptr, Capability pointer to describe the interrupt handler
 * @returns 0 on success, else -1
 */
static int
enable_irq(int irq, seL4_CPtr aep, seL4_CPtr *irq_handler_ptr)
{
    /* Create an IRQ handler */
    if (!(*irq_handler_ptr = cspace_irq_control_get_cap(cur_cspace, seL4_CapIRQControl, irq)))
        return -1;

    /* Assign to an endpoint */
    if (seL4_IRQHandler_SetEndpoint(*irq_handler_ptr, aep) != 0)
        return -1;

    /* Ack the handler before continuing */
    if (seL4_IRQHandler_Ack(*irq_handler_ptr) != 0)
        return -1;

    return 0;
}

/*
 * Initialise the timer.
 * @param void *mapped_vaddr, the address of the memory mapped registers for GPT
 */
void
init_timer(void *mapped_vaddr)
{
    gpt_virtual = mapped_vaddr;
    pq = init_pq();

    /* Setup pointers to registers of the GPT Timer */
    control_register_ptr = (seL4_Word *)(gpt_virtual + GPT_CR);
    prescale_register_ptr = (seL4_Word *)(gpt_virtual + GPT_PR);
    interrupt_register_ptr = (seL4_Word *)(gpt_virtual + GPT_IR);
    status_register_ptr = (seL4_Word *)(gpt_virtual + GPT_SR);
    compare_register_ptr = (seL4_Word *)(gpt_virtual + GPT_OCR1);
    upper_timestamp_register_ptr = (seL4_Word *)(gpt_virtual + GPT_OCR2); /* Use OCR2 for upper 32 bits of timestamp */
    counter_register_ptr = (seL4_Word *)(gpt_virtual + GPT_CNT); 
}

/*
 * Start the timer.
 * Enable interrupt requests and set GPT registers.
 * @param seL4_CPtr interrupt_ep, badged async endpoint that driver uses for delivering interrupts
 * @returns CLOCK_R_OK iff successful.
 */
int
start_timer(seL4_CPtr interrupt_ep)
{
    /* Stop timer if initialised */
    if (timer_started)
        stop_timer();

    /* Set timer interupts to be sent to interrupt_ep, and creates an interrupt capability */
    if (enable_irq(GPT_IRQ, interrupt_ep, &irq_handler) != 0)
        return CLOCK_R_FAIL; /* Operation failed for other reason (IRQ failed setup) */

    /* Reset key registers */
    *control_register_ptr = ZERO_RESET_VALUE;
    *prescale_register_ptr = ZERO_RESET_VALUE;
    *status_register_ptr = ZERO_RESET_VALUE;
    *interrupt_register_ptr = ZERO_RESET_VALUE;
    *compare_register_ptr = MAX_INT_RESET_VALUE;
    *upper_timestamp_register_ptr = ZERO_RESET_VALUE;

    /* Set the timer registers for settings */
    *control_register_ptr |= FREE_RUN_MASK; /* Free-Run mode */
    *control_register_ptr |= PERIPHERAL_CLOCK_MASK; /* peripheral clock */
    *control_register_ptr |= STOP_MODE_MASK; /* stop mode */
    *control_register_ptr |= DOZE_MODE_MASK; /* doze mode */
    *control_register_ptr |= WAIT_MODE_MASK; /* wait mode */
    *control_register_ptr |= DEBUG_MODE_MASK; /* debug mode */

    *interrupt_register_ptr |= ROLL_OVER_MASK; /* Roll over enabled */
    *interrupt_register_ptr |= OUTPUT_COMPARE_MASK; /* Output compare channel 1 enabled */

    /* Since clock is 66Mhz, prescale of 66 means the counter increases by 1 every 1/1,000,000 a second */
    /* Which is equivalent to counting microseconds */
    *prescale_register_ptr = GPT_PERIPHERAL_CLOCK_FREQUENCY;

    /* Start the clock */
    *control_register_ptr |= 1;
    timer_started = 1;

    return CLOCK_R_OK;
}

/*
 * Register a callback to be called after a given delay
 * @param uint64_t delay, microsecond delay before event
 * @param timer_callback_t callback, function to be run after delay
 * @param void *data, data to be passed to the callback function
 * @return id of the timer event
 */
uint32_t
register_timer(uint64_t delay, timer_callback_t callback, void *data)
{
    return add_event_to_pq(delay, callback, data, SINGLE_EVENT, GEN_UID);
}

/*
 * Register a repeating callback to be run every delay microseconds
 * @param uint64_t delay, microsecond delay before event
 * @param timer_callback_t callback, function to be run after delay
 * @param void *data, data to be passed to the callback function
 * @return id of the timer event
 */
uint32_t
register_repeating_timer(uint64_t delay, timer_callback_t callback, void *data)
{
    return add_event_to_pq(delay, callback, data, REPEAT_EVENT, GEN_UID);
}

/*
 * Remove a previously registered callback by its ID
 * @param uint32_t id of the registered timer
 * @return CLOCK_R_OK iff successful.
 */
int
remove_timer(uint32_t id)
{
    if (!timer_started)
        return CLOCK_R_UINT; /* Driver not initialised */

    if (!pq_remove(pq, id))
        return CLOCK_R_FAIL; /* Operation failed for other reason (invalid id) */

    /* The timer we just removed could have been the next event so we need to update the compare reg */
    *compare_register_ptr = pq_time_peek(pq);

    return CLOCK_R_OK;
}

/*
 * Handle an interrupt message sent to 'interrupt_ep' from start_timer
 * @returns CLOCK_R_OK iff successful
 */
int
timer_interrupt(void)
{
    if (*status_register_ptr & ROLL_OVER_MASK) {
        /* Overflow wont happen for ~584 years, we plan to finish AOS by then */
        *upper_timestamp_register_ptr += 1;
        *status_register_ptr |= ROLL_OVER_MASK; /* Acknowledge a roll over event occured */
    }

    if (*status_register_ptr & OUTPUT_COMPARE_MASK) {
        /* Run all callback events that should have already happened */
        /* Because multiple interrupts can bascially happen at the same time, so we need to account for that */
        do {
            event *cur_event = pq_pop(pq);
            if (!cur_event)
                return CLOCK_R_FAIL; /* This should only happen if malloc returned NULL */

            if (cur_event->callback)
                cur_event->callback(cur_event->uid, cur_event->data);

            /* Add event back to queue if repeating */
            if (cur_event->repeat)
                add_event_to_pq(cur_event->delay, cur_event->callback, cur_event->data, REPEAT_EVENT, cur_event->uid);

            free(cur_event);
        } while (!pq_is_empty(pq) && pq_time_peek(pq) <= time_stamp() + 1000); /* 1ms buffer */


        if (pq_is_empty(pq)) {
            /* We disable compare interrupts because theres no more events */
            *interrupt_register_ptr &= ~(OUTPUT_COMPARE_MASK); /* Output compare channel 1 disabled */
        } else {
            /* Set the next compare value to the event at the front of the priority queue */
            *compare_register_ptr = pq_time_peek(pq);
        }

        *status_register_ptr |= OUTPUT_COMPARE_MASK; /* Acknowledge a compare event occured */
    }

    /* Acknowledge the interrupt so more can happen */
    if (seL4_IRQHandler_Ack(irq_handler) != 0)
        return CLOCK_R_FAIL; /* Operation failed for other reason (Failed to ack interrupt) */

    return CLOCK_R_OK;   
}

/*
 * Get the present time in microseconds since booting.
 * @returns 64 bit timestamp on success, else negative value
 */
timestamp_t
time_stamp(void)
{
    if (!timer_started)
        return CLOCK_R_UINT; /* Driver not initialised */

    // TODO: HANDLE RACE CONDITION IN TICKING OVER
    return (timestamp_t)join32to64(*upper_timestamp_register_ptr, *counter_register_ptr);
}

/*
 * Stop clock driver operation.
 * @returns CLOCK_R_OK iff successful.
 */
int
stop_timer(void)
{
    /* Stop the clock */
    *control_register_ptr &= ~1;

    /* Clear the handler */
    seL4_IRQHandler_Clear(irq_handler);

    /* Delete the capability from the cspace */
    cspace_delete_cap(cur_cspace, irq_handler);

    /* Remove all upcoming events from the queue */
    pq_purge(pq);

    timer_started = 0;

    return CLOCK_R_OK;
}

/* Joins two 32 bit numbers into a 64 bit number and returns upper:lower */
static inline uint64_t
join32to64(uint32_t upper, uint32_t lower)
{
    return (uint64_t)lower | ((uint64_t)upper << 32);
}

/* adds an event to the PQ */
static uint32_t
add_event_to_pq(uint64_t delay, timer_callback_t callback, void *data, uint8_t repeat, uint32_t uid)
{
    if (!timer_started)
        return CLOCK_R_OK; /* Return 0 on failure */

    /* Turn interrupts back on, there will be a non empty PQ */
    if (pq_is_empty(pq))
        *interrupt_register_ptr |= OUTPUT_COMPARE_MASK; /* Output compare channel 1 enabled */

    int id = pq_push(pq, time_stamp() + delay, delay, callback, data, repeat, uid);

    /* If there was an issue pushing event, terminate early */
    if (!id)
        return EVENT_FAIL;

    /* The event might be at the front of the queue, in that case we need to update the compare reg */
    *compare_register_ptr = pq_time_peek(pq);

    return id;
}
