/* @LICENSE(NICTA) */

#ifndef _SYSCALL_SEL4_H_
#define _SYSCALL_SEL4_H_

#include <stdarg.h>

#define SYSCALL_MUSLC_NUM 378

typedef long (*muslc_syscall_t)(va_list);
typedef muslc_syscall_t muslc_syscall_array_t[378];
extern muslc_syscall_array_t *__muslc_syscall_ptr_table;

#endif /* _SYSCALL_SEL4_H_ */
