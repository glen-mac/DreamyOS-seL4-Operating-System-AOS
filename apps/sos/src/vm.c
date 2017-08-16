/*
 * Virtual memory implementation
 * Cameron Lonsdale & Glenn McGuire
 *
 */

#include "vm.h"
#include "mapping.h"
#include "proc.h"

int 
vm_fault(seL4_Word fault_addr, seL4_Word pc, seL4_Word fault_type)
{
	sos_map_page(fault_addr, curproc.vroot);
}