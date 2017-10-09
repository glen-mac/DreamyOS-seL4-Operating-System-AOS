/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */


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



#define PAGING_DEMO_PAGES_PER_BLOCK 16
#define PAGING_DEMO_BLOCKS 256

/* 
	This program tests swapping.  It mallocs a large amount of frames to induce
	paging to disk, then it tries writing to the frames to ensure that the data 
	hasn’t been corrupted.
*/

int main(void) {

    int page_count = PAGING_DEMO_BLOCKS * PAGING_DEMO_PAGES_PER_BLOCK;
    printf("Going to allocate around %d (0x%08x) pages\n", page_count, page_count);
    char **blocks = malloc(sizeof(char *) * PAGING_DEMO_BLOCKS);
    for (int i = 0; i < PAGING_DEMO_BLOCKS; i++) {
        blocks[i] = malloc(sizeof(char) * PAGE_SIZE * PAGING_DEMO_PAGES_PER_BLOCK);
        assert(blocks[i] != NULL);
        printf("Block %d@0x%08x\n", i, (unsigned int) blocks[i]);
    }
    printf("First round of pings\n");
    for (int block = 0; block < PAGING_DEMO_BLOCKS; block++) {
        for (int page = 0; page < PAGING_DEMO_PAGES_PER_BLOCK; page++) {
            // ping the middle of some frames
            char *addr = blocks[block] + (page * PAGE_SIZE) + (PAGE_SIZE/2);
            printf("Writing block %d page %d @ 0x%08x\n", block, page, (unsigned int) addr);
            char in = (page + 1) * (block + 1);
            *addr = in;
            assert(*addr == in);
        }
    }
    printf("Second round of pings\n");
    for (int block = 0; block < PAGING_DEMO_BLOCKS; block++) {
        for (int page = 0; page < PAGING_DEMO_PAGES_PER_BLOCK; page++) {
            char *addr = blocks[block] + (page * PAGE_SIZE) + (PAGE_SIZE/2);
            printf("Reading block %d page %d @ 0x%08x\n", block, page, (unsigned int) addr);
            char out = (page + 1) * (block + 1);
            assert(*addr == out);
        }
    }
    for (int block = 0; block < PAGING_DEMO_BLOCKS; block++) {
        free(blocks[block]);
    }
    return 0;

}
