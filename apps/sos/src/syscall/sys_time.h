/*
 * Time Syscalls
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#ifndef _SYS_TIME_H_
#define _SYS_TIME_H_

#include <sel4/sel4.h>

/* 
 * Syscall for usleep
 * msg(1) microsecond delay
 * @returns nwords in return message
 */
seL4_Word syscall_usleep(void);

/*
 * Syscall for time_stamp
 * returns 64 bit number in two 32 bits words
 * @returns nwords in return message
 */
seL4_Word syscall_time_stamp(void);

#endif /* _SYS_TIME_H_ */
