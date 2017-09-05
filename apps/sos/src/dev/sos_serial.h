/*
 * Wrappers for the serial device
 * 
 * Glenn McGuire & Cameron Lonsdale
 */

#ifndef _SOS_SERIAL_H_
#define _SOS_SERIAL_H_

#include <vfs/vfs.h>
#include <sys/uio.h>

/*
 * Intialise the console device for sos use
 * Creates a vnode for the device and registers that into the VFS
 * @returns 0 on success else 1
 */
int sos_serial_init(void);

/*
 * Write to the serial device
 * @param node, the vnode of the device
 * @param iov, the io vector
 * @returns nbytes written on success else errno
 */
int sos_serial_write(vnode *node, uiovec *iov);

/*
 * Read from the serial device
 * @param node, the vnode of the device
 * @param iov, the io vector
 * @returns nbytes read on success else errno
 */
int sos_serial_read(vnode *node, uiovec *iov);

/*
 * Close a serial device
 * @param node, the vnode of the device
 * @returns 0 on success else 1
 */
int sos_serial_close(vnode *node);

/*
 * Stat the console
 * @param node, the vnode of the device
 * @oaram buf, the stat buffer to write to
 * @returns 0 on success else 1
 */
int sos_serial_stat(vnode *node, sos_stat_t **buf);

#endif /* _SOS_SERIAL_H_ */