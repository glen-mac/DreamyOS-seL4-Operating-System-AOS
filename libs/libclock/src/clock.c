/*
 * Clock Implementation
 *
 * Cameron Lonsdale & Glenn McGuire
 *
 */

#include <clock/clock.h>
#include <clock/pq.h>

#include <cspace/cspace.h>
#include <utils/util.h>

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
#define FREE_RUN_MASK BIT(9)
#define PERIPHERAL_CLOCK_MASK BIT(6)
#define ENMOD_MODE_MASK BIT(1)
#define ROLL_OVER_MASK BIT(5)
#define OUTPUT_COMPARE_MASK BIT(0)

/* Definitions if a timer event is single use or repeating */
#define REPEAT_EVENT 1
#define SINGLE_EVENT 0

/* Flags to specify if an event failed and if a UID needs to be generated */
#define EVENT_FAIL 0
#define GEN_UID 0

/* Global var for the start of GPT mapped memory */
static void *gpt_virtual = NULL;

/* Global vars for the GPT timer registers */
static seL4_Word *control_register_ptr;
static seL4_Word *prescale_register_ptr;
static seL4_Word *interrupt_register_ptr;
static seL4_Word *status_register_ptr;
static seL4_Word *compare_register_ptr;
static seL4_Word *upper_timestamp_register_ptr;
static seL4_Word *counter_register_ptr;

static seL4_CPtr irq_handler; /* Global IRQ handler */
static priority_queue *pq = NULL; /* Timer event pq handler */
static uint32_t timer_started = 0; /* Boolean to state if the timer has started or not */

static int enable_irq(int irq, seL4_CPtr aep, seL4_CPtr *irq_handler_ptr);
static uint64_t join32to64(uint32_t upper, uint32_t lower);
static uint32_t add_event_to_pq(uint64_t delay, timer_callback_t callback, void *data, uint8_t repeat, uint32_t uid);
static void check_for_rollover(void);

void
init_timer(void *vaddr)
{
    gpt_virtual = vaddr;
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

int
start_timer(seL4_CPtr interrupt_ep)
{
    /* Stop timer if initialised */
    if (timer_started)
        stop_timer();

    /* Set timer interupts to be sent to interrupt_ep, and creates an interrupt capability */
    if (enable_irq(GPT_IRQ, interrupt_ep, &irq_handler) != 0) {
        LOG_INFO("Failed to enable irq");
        return CLOCK_R_FAIL; /* Operation failed for other reason (IRQ failed setup) */
    }

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
    *control_register_ptr |= ENMOD_MODE_MASK; /* Counter will reset to 0 when GPT disabled */

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

uint32_t
register_timer(uint64_t delay, timer_callback_t callback, void *data)
{
    return add_event_to_pq(delay, callback, data, SINGLE_EVENT, GEN_UID);
}

uint32_t
register_repeating_timer(uint64_t delay, timer_callback_t callback, void *data)
{
    return add_event_to_pq(delay, callback, data, REPEAT_EVENT, GEN_UID);
}

int
remove_timer(uint32_t id)
{
    if (!timer_started) {
        LOG_ERROR("Driver uninitialised");
        return CLOCK_R_UINT; /* Driver not initialised */
    }

    if (!pq_remove(pq, id)) {
        LOG_ERROR("Invalid timer id: %d", id);
        return CLOCK_R_FAIL; /* Operation failed for other reason (invalid id) */
    }

    /* The timer we just removed could have been the next event so we need to update the compare reg */
    *compare_register_ptr = pq_time_peek(pq);

    return CLOCK_R_OK;
}

int
timer_interrupt(void)
{
    if (!timer_started) {
        LOG_ERROR("Driver uninitialised");
        return CLOCK_R_UINT; /* Driver not initialised */
    }

    check_for_rollover();

    if (*status_register_ptr & OUTPUT_COMPARE_MASK) {
        /* 
         * Run all callback events that should have already happened
         * Because multiple interrupts can bascially happen at the same time, so we need to account for that
         */
        do {
            event *cur_event = pq_pop(pq);
            if (!cur_event) {
                LOG_ERROR("pq_pop returned null");
                return CLOCK_R_FAIL; /* This should only happen if malloc returned NULL */
            }

            if (cur_event->callback)
                cur_event->callback(cur_event->uid, cur_event->data);

            /* Add event back to queue if repeating */
            if (cur_event->repeat)
                add_event_to_pq(cur_event->delay, cur_event->callback, cur_event->data, REPEAT_EVENT, cur_event->uid);

            free(cur_event);
            /* Check rollover every time */
            check_for_rollover();
        } while (!pq_is_empty(pq) && pq_time_peek(pq) <= time_stamp() + (uint64_t)MILLISECONDS(1));

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
    if (seL4_IRQHandler_Ack(irq_handler) != 0) {
        LOG_ERROR("Failed to acknowledge interrupt");
        return CLOCK_R_FAIL; /* Operation failed for other reason (Failed to ack interrupt) */
    }

    return CLOCK_R_OK;   
}

timestamp_t
time_stamp(void)
{
    if (!timer_started) {
        LOG_ERROR("Driver uninitialised");
        return CLOCK_R_UINT; /* Driver not initialised */
    }

    /*
     * There can be a race condition here where between reading the lower and upper 32 bits
     * The data can change. In particular there can be two cases:
     *
     * 1. We read upper bits first, but then lower ticks over and we end up reading a value that 
     * is in the past, because upper will have stayed the same but lower is now 0x00000000 etc.
     * 
     * 2. We read lower bits first (close to 0xFFFFFFFF), then upper ticks over before we read. 
     * And we see a value that is very far into the future.
     */
    timestamp_t first = (timestamp_t)join32to64(*upper_timestamp_register_ptr, *counter_register_ptr);
    timestamp_t second = (timestamp_t)join32to64(*upper_timestamp_register_ptr, *counter_register_ptr);

    /* If second is less than first, the second timestamp is in the past, which is condition (1) */
    if (second < first) {
        /* This timestamp shold not be incorrect as we just had the race condition */
        check_for_rollover(); /* This is required because we might be in an interrupt handler and roll over didnt get recognised yet */
        LOG_INFO("64-bit timer race condition detected");
        return (timestamp_t)join32to64(*upper_timestamp_register_ptr, *counter_register_ptr);
    }

    /* If the upper 32 bits mismatch then a rollover occured and its POSSIBLE the condition (2) is met */
    if (UPPER32BITS(first) != UPPER32BITS(second)) {
        /* This timestamp shold not be incorrect as we just had the race condition */
        check_for_rollover();
        LOG_INFO("64-bit timer race condition detected");
        return (timestamp_t)join32to64(*upper_timestamp_register_ptr, *counter_register_ptr);
    }

    /* We have caught all the possibilites of the condition above, so second is now correct to return */
    return second;
}

int
stop_timer(void)
{
    /* Stop the clock */
    *control_register_ptr &= ~1;

    /* Clear the handler */
    if (seL4_IRQHandler_Clear(irq_handler) != 0) {
        LOG_ERROR("Failed to clear timer IRQ");
        return CLOCK_R_FAIL;
    }

    /* Delete the capability from the cspace */
    if (cspace_delete_cap(cur_cspace, irq_handler) != CSPACE_NOERROR) {
        LOG_ERROR("Failed to delete irq cap");
        return CLOCK_R_FAIL;
    }

    /* Remove all upcoming events from the queue */
    pq_purge(pq);

    timer_started = 0;

    return CLOCK_R_OK;
}

/* 
 * Enable Interrupts for a sepcific IRQ and endpoint
 * This is copied from network.c (and modified)
 * @param irq, interrupt request id
 * @param aep, asynchronous endpoint the interrupt message will be delivered to
 * @param[out] irq_handler_ptr, Capability pointer to describe the interrupt handler
 * @returns 0 on success, else 1
 */
static int
enable_irq(int irq, seL4_CPtr aep, seL4_CPtr *irq_handler_ptr)
{
    /* Create an IRQ handler */
    if (!(*irq_handler_ptr = cspace_irq_control_get_cap(cur_cspace, seL4_CapIRQControl, irq))) {
        LOG_ERROR("Failed to create an IRQ handler");
        return 1;
    }

    /* Assign to an endpoint */
    if (seL4_IRQHandler_SetEndpoint(*irq_handler_ptr, aep) != 0) {
        LOG_ERROR("Failed to assign handler to endpoint");
        return 1;
    }

    /* Ack the handler before continuing */
    if (seL4_IRQHandler_Ack(*irq_handler_ptr) != 0) {
        LOG_ERROR("Failed to acknowledge handler");
        return 1;
    }

    return 0;
}

/*
 * Joins two 32 bit numbers into a 64 bit number.
 * @param upper, the upper 32 bits
 * @param lower, the lower 32 bits
 * @returns 64 bit number upper:lower
 */
static inline uint64_t
join32to64(uint32_t upper, uint32_t lower)
{
    return (uint64_t) upper << 32 | lower;
}

/* 
 * Add an event to the PQ.
 * @param delay, the microsecond delay until the event
 * @param callback, the callback to run at the event
 * @param data, the data to provide to the callback
 * @param repeat, flag to specify if the event is repeating
 * @param uid, unique id of the event
 * @returns 0 on failure, else positive id
 */
static uint32_t
add_event_to_pq(uint64_t delay, timer_callback_t callback, void *data, uint8_t repeat, uint32_t uid)
{
    uint32_t id;

    if (!timer_started) {
        LOG_ERROR("Driver uninitialised");
        return CLOCK_R_OK; /* Return 0 on failure */
    }

    /* If the event will happen within 1ms, just execute the event, as we might miss it */
    if (delay < (uint64_t)MILLISECONDS(1)) {
        if ((id = pq_get_next_id(pq)) == 0) {
            LOG_ERROR("Failed to acquire next priority queue id");
            return EVENT_FAIL;
        }

        if (callback) 
            callback(id, data);

        return id;
    }

    /* Turn interrupts back on, there will be a non empty PQ */
    if (pq_is_empty(pq))
        *interrupt_register_ptr |= OUTPUT_COMPARE_MASK; /* Output compare channel 1 enabled */

    /* If there was an issue pushing event, terminate early */
    if ((id = pq_push(pq, time_stamp() + delay, delay, callback, data, repeat, uid)) == 0) {
        LOG_ERROR("Failed to add event to the queue");
        return EVENT_FAIL;
    }

    /* The event might be at the front of the queue, in that case we need to update the compare reg */
    *compare_register_ptr = pq_time_peek(pq);

    return id;
}

/*
 * Check if a rollover interrupt occured
 */
static void
check_for_rollover(void)
{
    if (*status_register_ptr & ROLL_OVER_MASK) {
        /* Overflow wont happen for ~584 years, we plan to finish AOS by then */
        *upper_timestamp_register_ptr = *upper_timestamp_register_ptr + 1;
        *status_register_ptr |= ROLL_OVER_MASK; /* Acknowledge a roll over event occured */ 
        LOG_INFO("32-bit timer rollover");
    }
}
