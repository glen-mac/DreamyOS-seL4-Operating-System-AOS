/*
 * Time Syscall handler
 *
 * Glenn McGuire & Cameron Lonsdale
 */


#include "sys_time.h"

#include <clock/clock.h>
#include <coro/picoro.h>
#include "syscall.h"
#include <utils/time.h>
#include <utils/util.h>

static void callback_sys_usleep(uint32_t id, void *data);

seL4_Word
syscall_usleep(void)
{
    uint32_t ms_delay = seL4_GetMR(1);
    LOG_INFO("syscall: thread made sos_usleep(%d)", ms_delay);
    assert(register_timer(MILLISECONDS(ms_delay), callback_sys_usleep, NULL) != 0);

    /* Unblock the process when timer callback is called */ 
    yield(NULL);
    return 0;
}

seL4_Word
syscall_time_stamp(void)
{
    LOG_INFO("syscall: thread made sos_time_stamp()");

    timestamp_t ts = time_stamp();
    seL4_SetMR(0, UPPER32BITS(ts));
    seL4_SetMR(1, LOWER32BITS(ts));
    return 2;
}

/*
 * The callback for a usleep syscall
 */
void
callback_sys_usleep(uint32_t id, void *data)
{
    resume(syscall_coro, NULL);
}
