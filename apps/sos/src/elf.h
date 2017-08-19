/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _LIBOS_ELF_H_
#define _LIBOS_ELF_H_

#include <sel4/sel4.h>
#include "addrspace.h"

/*
 * Load an elf binary into an address space
 * @param as, the destination address space
 * @param elf_file, name of the elf binary
 * @returns 0 on success, else error
 */
int elf_load(addrspace *as, char *elf_file);

#endif /* _LIBOS_ELF_H_ */
