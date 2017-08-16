/*
 * Virtual memory interface
 * Cameron Lonsdale & Glenn McGuire
 *
 */

#ifndef _VM_H_
#define _VM_H_

#include <sel4/sel4.h>

/* 
 * Handle a vm fault.
 * @param fault_addr, the address the application faulted on
 * @param pc, the program counter the application was on
 * @param fault_type, the type of the fault
 * @param fault_cause, Architecture encoded register specifying fault cause
 */ 
void vm_fault(seL4_Word fault_addr, seL4_Word pc, seL4_Word fault_type, seL4_Word fault_cause);

#endif /* _VM_H_ */