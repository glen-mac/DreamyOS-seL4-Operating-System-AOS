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

/*
 * Operations for a serial vnode
 */
static const vnode_ops serial_vnode_ops = {
    //.vop_open = sos_serial_open,
    //.vop_close = sos_serial_close
    //.vop_read = sos_serial_read,
    //.vop_write = sos_serial_write,
    //.vop_lookup = NULL,
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
