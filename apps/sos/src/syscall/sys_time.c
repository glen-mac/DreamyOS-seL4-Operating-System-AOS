/*
 * Time Syscall handler
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include "syscall.h"
#include "sys_time.h"
#include <sos.h>
#include <cspace/cspace.h>
#include <utils/time.h>
#include <utils/util.h>
#include <clock/clock.h>

/*
 * The callback for a usleep syscall
 */
void
callback_sys_usleep(uint32_t id, void *reply_cap)
{
    seL4_MessageInfo_t reply = seL4_MessageInfo_new(0, 0, 0, 0);
    seL4_Send((seL4_CPtr)reply_cap, reply); /* Send back to user to unblock the process */
}

void
syscall_usleep(seL4_CPtr reply_cap)
{
    LOG_INFO("syscall: thread made sos_usleep");

    uint32_t ms_delay = seL4_GetMR(1);
    register_timer(ms_delay * US_IN_MS, callback_sys_usleep, (void *)reply_cap);
    /* Unblock the process when timer callback is called */
}

void
syscall_time_stamp(seL4_CPtr reply_cap)
{
    LOG_INFO("syscall: thread made sos_time_stamp");

    seL4_MessageInfo_t reply = seL4_MessageInfo_new(0, 0, 0, 2);
    timestamp_t ts = time_stamp();
    seL4_SetMR(0, (ts & 0xFFFFFFFF00000000L) >> 32);
    seL4_SetMR(1, ts & 0xFFFFFFFFL);
    seL4_Send(reply_cap, reply);
}
