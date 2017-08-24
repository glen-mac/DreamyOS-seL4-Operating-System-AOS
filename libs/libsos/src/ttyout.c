/*
 * tty console IO
 *
 * Glenn McGuire & Cameron Lonsdale
 */

#include "ttyout.h"

#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <sos.h>
#include <unistd.h>

#include <sel4/sel4.h>

size_t
sos_write(void *data, size_t count)
{
    return sos_sys_write(STDOUT_FILENO, data, count);
}
