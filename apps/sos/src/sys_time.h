/*
 * Time Syscall handler
 *
 * Glenn McGuire & Cameron Lonsdale
 */

/* syscall for usleep */
void syscall_usleep(seL4_CPtr);

/* syscall for time_stamp */
void syscall_time_stamp(seL4_CPtr);
