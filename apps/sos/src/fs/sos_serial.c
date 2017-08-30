/*
 * Wrappers for the serial device
 * 
 * Glenn McGuire & Cameron Lonsdale
 */

#include "sos_serial.h"
#include "picoro.h"
#include <vfs/vfs.h>
#include <vfs/device.h>

#include <syscall/syscall.h>

#include <stdlib.h>
#include <serial/serial.h>

#include <utils/util.h>
#include "ringbuf.h"

uiovec *global_uio = NULL;
int nbytes_read = 0;

ring_buffer_t *input_buffer = NULL;

/*
 * Operations for a serial vnode
 */
static const vnode_ops serial_vnode_ops = {
    /* Dont really need open as device vnodes are already open when they are registered */
    .vop_close = sos_serial_close,
    .vop_read = sos_serial_read,
    .vop_write = sos_serial_write,
};

static void
handler(struct serial *serial, char c)
{
    /* If a user has registered data, then give it straight to them */
    if (global_uio) {
        char *buf = (char *)global_uio->uiov_base;
        buf[nbytes_read] = c;
        nbytes_read++;
        if (c == '\n' || nbytes_read >= global_uio->uiov_len) {
            LOG_INFO("resuming blocked coro");
            resume(syscall_coro, NULL);
        }

    } else if (!ring_buffer_is_full(input_buffer)) {
        /* Otherwise we buffer it */
        ring_buffer_queue(input_buffer, c);
    }
}

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

    input_buffer = malloc(sizeof(ring_buffer_t));
    ring_buffer_init(input_buffer);

    serial_register_handler(port, handler);

    return 0;
}

int
sos_serial_write(vnode *node, uiovec *iov)
{
    struct serial *port = node->vn_data;
    return serial_send(port, iov->uiov_base, iov->uiov_len);
}

int
sos_serial_read(vnode *node, uiovec *iov)
{
    char *user_buf = iov->uiov_base;
    int bytes_read = MIN(ring_buffer_num_items(input_buffer), iov->uiov_len);
    for (int i = 0; i < bytes_read; ++i) {
        assert(ring_buffer_dequeue(input_buffer, user_buf + i) == 1);
        if (user_buf[i] == '\n') {
            bytes_read = i + 1;
            goto read_return;
        }
    }

    if (bytes_read == iov->uiov_len)
        goto read_return;

    /* Need to read more, this is blocking */
    nbytes_read = bytes_read;
    global_uio = iov;

    yield(NULL); /* Yield back to event_loop, will be resumed when there is data */

    global_uio = NULL;
    bytes_read = nbytes_read;
    nbytes_read = 0;

    read_return:
        return bytes_read;
}

int
sos_serial_close(vnode *node)
{   
    /* 
     * We dont want to close the device as that would unintiailise it
     * The vnode is permanent and lives as a registered device so we dont want to free it
     */
    return 0;
}