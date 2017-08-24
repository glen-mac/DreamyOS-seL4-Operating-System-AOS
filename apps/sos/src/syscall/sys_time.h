/*
 * Time Syscall handler
 *
 * Glenn McGuire & Cameron Lonsdale
 */

/* 
 * Syscall for usleep
 * msg(1) microsecond delay
 */
void syscall_usleep(seL4_CPtr reply_cap);

/*
 * Syscall for time_stamp
 * returns 64 bit number in two 32 bits words
 */
void syscall_time_stamp(seL4_CPtr reply_cap);
