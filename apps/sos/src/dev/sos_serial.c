/*
 * Wrappers for the serial device
 * 
 * Glenn McGuire & Cameron Lonsdale
 */

#include "sos_serial.h"

#include <coro/picoro.h>
#include <fcntl.h>
#include <vfs/vfs.h>
#include <vfs/device.h>
#include <serial/serial.h>
#include <stdlib.h>
#include <syscall/syscall.h>
#include <utils/util.h>
#include <utils/ringbuf.h>

/* Pointer to waititng struct to be filled by read in data */
static uiovec *volatile global_uio = NULL;

/* Number of bytes read by the console */
static volatile int nbytes_read = 0;

/* Input buffer */
static ring_buffer_t *volatile input_buffer = NULL;

/* Status struct for the console */
static sos_stat_t *stat = NULL;

/*
 * Operations for a serial vnode
 */
static const vnode_ops serial_vnode_ops = {
    .vop_open = sos_serial_open,
    .vop_close = sos_serial_close,
    .vop_read = sos_serial_read,
    .vop_write = sos_serial_write,
    .vop_stat = sos_serial_stat,
};

/*
 * Interrupt handler for a received character
 * @param serial, the serial structure
 * @param c, the receieved character
 */
static void
handler(struct serial *serial, char c)
{
    /* If a user has registered data, then give it straight to them */
    if (global_uio) {
        char *buf = (char *)global_uio->uiov_base;
        buf[nbytes_read] = c;
        nbytes_read++;

        /* New line or required bytes read restarts the blocked process */
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
    if (port == NULL) {
        LOG_ERROR("Failed to initialise the serial device");
        return 1;
    }

    vnode *node = vnode_create(port, &serial_vnode_ops, 0, 0);
    if (node == NULL) {
        LOG_ERROR("Failed to create the vnode");
        return 1;
    }

    if (device_register("console", node) != 0) {
        LOG_ERROR("Failed to register console as a device");
        free(node);
        return 1;
    }

    if ((input_buffer = malloc(sizeof(ring_buffer_t))) == NULL) {
        LOG_ERROR("Failed to create the input buffer");
        free(node);
        return 1;
    }
    ring_buffer_init(input_buffer);

    if ((stat = malloc(sizeof(sos_stat_t))) == NULL) {
        LOG_ERROR("Failed to create the serial status structure");
        return 1;
    }

    stat->st_type = ST_SPECIAL;
    stat->st_fmode = FM_READ | FM_WRITE;
    stat->st_size = 0; /* Because theres not really a size */
    stat->st_ctime = 0; /* Created on init */
    stat->st_atime = 0;

    serial_register_handler(port, handler);

    return 0;
}

int
sos_serial_open(vnode *vnode, fmode_t mode)
{
    /* Update reader / write count based on mode */
    if (mode == O_RDONLY || mode == O_RDWR) {
        if (vnode->readcount > 0) {
            LOG_ERROR("Enforcing single reader policy on serial");
            return 1;
        }
        vnode->readcount += 1;
        if (mode == O_RDONLY)
            return 0;
    }
    /* O_RDWR or O_WRONLY */
    vnode->writecount += 1;

    /* Update access timestamp */
    stat->st_atime = U_TO_MS(time_stamp());

    return 0;
}

int
sos_serial_write(vnode *node, uiovec *iov)
{
    return serial_send(node->vn_data, (char *)iov->uiov_base, (int)iov->uiov_len);
}

int
sos_serial_read(vnode *node, uiovec *iov)
{
    /* Enforce single reader policy */
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

    /* Read number of required bytes from the ring buffer */
    if (bytes_read == iov->uiov_len)
        goto read_return;

    /* Need to read more, this is blocking */
    nbytes_read = bytes_read;
    global_uio = iov;
    
    /* Save the current coroutine */
    ((struct serial *)(node->vn_data))->routine = (void *)coro_getcur();   
    yield(NULL); /* Yield back to event_loop, will be resumed when there is data */

    /* Reset the global state */
    global_uio = NULL;
    bytes_read = nbytes_read;
    nbytes_read = 0;

    read_return:
        return bytes_read;
}

int
sos_serial_close(vnode *node, fmode_t mode)
{   
    /* 
     * We dont want to close the device as that would unintiailise it
     * The vnode is permanent and lives as a registered device so we dont want to free it
     */
    if (mode == O_RDONLY) {
        node->readcount -= 1;
    } else if (mode == O_RDWR) {
        node->readcount -= 1;
        node->writecount -= 1;
    } else if (mode == O_WRONLY) {
        node->writecount -= 1;
    } else {
        /* Invalid mode */
        return 1;
    }

    return 0;
}

int
sos_serial_stat(vnode *node, sos_stat_t **buf)
{
    *buf = stat;
    return 0;
}
