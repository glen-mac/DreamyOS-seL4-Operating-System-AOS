#include <assert.h>
#include <fcntl.h>
#include <sel4/sel4.h>
#include <sos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ttyout.h>
#include <unistd.h>
#include <utils/page.h>

int
main(void)
{
    size_t npages = 100;
    int buffer[npages * MAX_IO_BUF];
    pid_t myid = sos_my_id();

    while(1) {
        for (int i = 0; i < npages; i++) {
            for (int j = 0; j < MAX_IO_BUF; ++j)
                buffer[(i * MAX_IO_BUF) + j] = (i * MAX_IO_BUF) + j;
        }

        for (int i = 0; i < npages; i++) {
            // Just to test everything is being copied correctly
            for (int j = 0; j < MAX_IO_BUF; ++j) {
                if (buffer[(i * MAX_IO_BUF) + j] != (i * MAX_IO_BUF) + j) {
                    printf("%d, %d is %d\n", i, j-2, buffer[(i * MAX_IO_BUF) + (j-2)]);
                    printf("%d, %d is %d\n", i, j-1, buffer[(i * MAX_IO_BUF) + (j-1)]);
                    printf("process %d: data was not what was written, expected %d, recieved %d at %d, %d\n",
                        myid, ((i * MAX_IO_BUF) + j), buffer[(i * MAX_IO_BUF) + j], i, j);
                    printf("receive is also %p\n", buffer[(i * MAX_IO_BUF) + j]);
                    printf("%d, %d is %d\n", i, j+1, buffer[(i * MAX_IO_BUF) + (j+1)]);
                    printf("%d, %d is %d\n", i, j+2, buffer[(i * MAX_IO_BUF) + (j+2)]);
                    assert(!"Failure");
                }
            }
        }
    }

    return 0;
}
