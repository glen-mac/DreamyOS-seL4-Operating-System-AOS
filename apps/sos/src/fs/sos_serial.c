/*
 * Wrappers for the serial device
 * 
 * Glenn McGuire & Cameron Lonsdale
 */

#include "sos_serial.h"

#include <vfs/vfs.h>
#include <vfs/device.h>

#include <stdlib.h>
#include <serial/serial.h>

#include <utils/util.h>

struct iovec *global_uio = NULL;
int nbytes_read = 0;

/*
 * Operations for a serial vnode
 */
static const vnode_ops serial_vnode_ops = {
    //.vop_open = sos_serial_open,
    //.vop_close = sos_serial_close
    .vop_read = sos_serial_read,
    .vop_write = sos_serial_write,
};

int
sos_serial_init(void)
{
    struct serial *port = serial_init();
    if (!port)
        return 1;

    vnode *node = malloc(sizeof(vnode));
    if (!node)
        return 1;

    node->vn_data = port;
    node->vn_ops = &serial_vnode_ops;

    if (device_register("console", node) != 0) {
        free(node);
        return 1;
    }

    return 0;
}

static void
handler(struct serial *serial, char c)
{
    LOG_INFO("c is %c", c);
    char *recv_buff = (char *)global_uio->iov_base;
    recv_buff[nbytes_read] = c;
    nbytes_read += 1;
    if (c == '\n' || nbytes_read == global_uio->iov_len) {
        LOG_INFO("new line OR nbytes read we must unblock the process");
        // Todo: go back to the coroutine stopped in sos_serial_read
    }
}

int
sos_serial_write(vnode *node, struct iovec *iov)
{
    struct serial *port = node->vn_data;
    return serial_send(port, iov->iov_base, iov->iov_len);
}

int
sos_serial_read(vnode *node, struct iovec *iov)
{
    struct serial *port = node->vn_data;
    global_uio = iov;
    serial_register_handler(port, handler);
    // TODO: block?, coroutine back to the syscall loop

    return nbytes_read;
}
