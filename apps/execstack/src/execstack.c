/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/* Simple shell to run on SOS */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <utils/time.h>

/* Your OS header file */
#include <sos.h>

#define PAGE_SIZE 4096
#define BUF_SIZ   (4 * PAGE_SIZE)
#define MAX_ARGS   32

/*
	This program tries to execute an address on the stack
	It should vm fault out and the process should be killed
*/


int main(void) {

    int stack_addr = 4;

    int (*functionPtr)(int,int);
    functionPtr =  &stack_addr;
    int sum = (*functionPtr)(1,1);

    printf("\n[New process]\n");


    printf("[Process exiting]\n");
    return 32;
}
