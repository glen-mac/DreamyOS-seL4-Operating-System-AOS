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

seL4_Word
syscall_usleep(void)
{
    uint32_t ms_delay = seL4_GetMR(1);
 
    LOG_INFO("syscall: thread made sos_usleep(%d)", ms_delay);

    LOG_INFO(">>> registering callback with usleep stack coro @ %p", coro_getcur());
    assert(register_timer(MILLISECONDS(ms_delay), callback_sys_usleep, (void *)coro_getcur()) != 0);

    /* Unblock the process when timer callback is called */ 
    int * retval = (int *)yield(NULL);

    LOG_INFO(">>> usleep coro returned to usleep!!!");
    LOG_INFO(">>> usleep return value was: %d", *retval);

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
    int * a =(int *)malloc(sizeof(int));
    *a = 1337;
    LOG_INFO(">>> usleep callback recv with cur_coro addr: @ %p\n", (coro)data);
    //create_event_cb(*(coro *)data, 1, (void *)a);   
    resume((coro)data, (void *)a);
}
