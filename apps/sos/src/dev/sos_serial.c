/*
 * Wrappers for the serial device
 * 
 * Glenn McGuire & Cameron Lonsdale
 */

#include "sos_serial.h"
#include <fcntl.h>
#include <coro/picoro.h>
#include <vfs/vfs.h>
#include <vfs/device.h>

#include <syscall/syscall.h>

#include <stdlib.h>
#include <serial/serial.h>

#include <utils/util.h>
#include <utils/ringbuf.h>

static uiovec *volatile global_uio = NULL;
static volatile int nbytes_read = 0;

static ring_buffer_t *volatile input_buffer = NULL;
static sos_stat_t *stat = NULL;

/*
 * Operations for a serial vnode
 */
static const vnode_ops serial_vnode_ops = {
    /* Dont really need open as device vnodes are already open when they are registered */
    .vop_close = sos_serial_close,
    .vop_read = sos_serial_read,
    .vop_write = sos_serial_write,
    .vop_stat = sos_serial_stat,
};

static void
handler(struct serial *serial, char c)
{
    /* If a user has registered data, then give it straight to them */
    if (global_uio) {
        char *buf = (char *)global_uio->uiov_base;
        buf[nbytes_read] = c;
        nbytes_read++;

        if (c == '\n' || nbytes_read >= global_uio->uiov_len)
            resume((coro)(serial->routine), NULL);

    } else if (!ring_buffer_is_full(input_buffer)) {
        /* Otherwise we buffer it */
        ring_buffer_queue(input_buffer, c);
    }
}

int
sos_serial_init(void)
{
    struct serial *port = serial_init();
    if (!port) {
        LOG_ERROR("serial_init failed");
        return 1;
    }

    vnode *node = malloc(sizeof(vnode));
    if (!node) {
        LOG_ERROR("malloc returned null");
        return 1;
    }

    node->vn_data = port;
    node->vn_ops = &serial_vnode_ops;

    if (device_register("console", node) != 0) {
        LOG_ERROR("device_register failed");
        free(node);
        return 1;
    }

    if ((input_buffer = malloc(sizeof(ring_buffer_t))) == NULL) {
        LOG_ERROR("malloc returned null");
        free(node);
        return 1;
    }

    ring_buffer_init(input_buffer);
    assert(serial_register_handler(port, handler) == 0);

    if ((stat = malloc(sizeof(sos_stat_t))) == NULL) {
        LOG_ERROR("malloc returned NULL");
        return 1;
    }

    stat->st_type = ST_SPECIAL;
    stat->st_fmode = FM_READ | FM_WRITE;
    stat->st_size = 0; /* Because theres not really a size */
    stat->st_ctime = 3; /* TODO: Grab the system timestamp and convert to ms */
    stat->st_atime = 4; /* TODO: update this on open() */

    return 0;
}

int
sos_serial_write(vnode *node, uiovec *iov)
{
    struct serial *port = node->vn_data;
    return serial_send(port, (char *)iov->uiov_base, (int)iov->uiov_len);
}

int
sos_serial_read(vnode *node, uiovec *iov)
{
    if (global_uio)
        return -1;

    char *user_buf = iov->uiov_base;
    seL4_Word bytes_read = MIN(ring_buffer_num_items(input_buffer), iov->uiov_len);

    /* Read from the buffer if there are stored bytes */
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
    
    ((struct serial *)(node->vn_data))->routine = (void *)coro_getcur();   
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

int
sos_serial_stat(vnode *node, sos_stat_t **buf)
{
    *buf = stat;
    return 0;
}
