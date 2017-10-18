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

#include <proc/proc.h>

/*
 * Load an elf binary into an address space
 * @param curproc, the destination process
 * @param app_name, name of the binary
 * @param[out] elf_pc, the program counter of the elf binary
 * @param[out] last_section, the last section in the binary
 * @returns 0 on success, else error
 */
int elf_load(proc *curproc, char *app_name, uint64_t *elf_pc, uint32_t *last_section);

#endif /* _LIBOS_ELF_H_ */
