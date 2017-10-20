/*
 * Time Syscall handler
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include "sys_time.h"

#include "event.h"
#include <clock/clock.h>
#include <coro/picoro.h>
#include "syscall.h"
#include <utils/time.h>
#include <utils/util.h>
#include <stdlib.h>

static void callback_sys_usleep(uint32_t id, void *data);

/* Flag to specify if the timer has fired */
volatile static bool timer_fired;

int
syscall_usleep(proc *curproc)
{
    int32_t ms_delay = seL4_GetMR(1);

    LOG_SYSCALL(curproc->pid, "sos_usleep(%d)", ms_delay);

    /* Only sleep for positive delays */
    if (ms_delay <= 0)
        return 0;

    timer_fired = FALSE;
    /* Register timer returns 0 on failure */
    if (register_timer(MILLISECONDS(ms_delay), callback_sys_usleep, (void *)coro_getcur()) == 0) {
        /* Only happens if out of memory */
        LOG_ERROR("Failed to register the timer");
        return 0;
    }

    /* Only yield if the event hasnt fired */
    if (!timer_fired)
        /* Unblock the process when timer callback is called */
        yield(NULL);

    return 0;
}

int
syscall_time_stamp(proc *curproc)
{
    LOG_SYSCALL(curproc->pid, "sos_time_stamp()");

    timestamp_t ts = time_stamp();
    seL4_SetMR(0, UPPER32BITS(ts));
    seL4_SetMR(1, LOWER32BITS(ts));
    return 2;
}

/*
 * The callback for a usleep syscall
 * Define the timer as fired
 * Resume the waiting coroutine if its been waiting
 */
void
callback_sys_usleep(uint32_t id, void *data)
{
    timer_fired = TRUE;
    if (resumable((coro)data))
        resume((coro)data, NULL);
}
