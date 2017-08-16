/*
 * Virtual memory interface
 * Cameron Lonsdale & Glenn McGuire
 *
 */

#ifndef _VM_H_
#define _VM_H_

#include <sel4/sel4.h>

int vm_fault(seL4_Word fault_addr, seL4_Word pc, seL4_Word fault_type);

#endif /* _VM_H_ */