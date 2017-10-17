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
 * Intialise the console device for use by SOS
 * Creates a vnode for the device and registers with the VFS
 * @returns 0 on success else 1
 */
int sos_serial_init(void);

/*
 * Open the serial device
 * Enforces single reader policy
 * @param vnode, vnode of the device
 * @param mode, mode of access
 * @returns 0 on success else 1
 */
int sos_serial_open(vnode *vnode, fmode_t mode);

/*
 * Write to the serial device
 * @param node, the vnode of the device
 * @param iov, the io vector
 * @returns nbytes written
 */
int sos_serial_write(vnode *node, uiovec *iov);

/*
 * Read from the serial device
 * @param node, the vnode of the device
 * @param iov, the io vector
 * @returns nbytes read on success else -1
 */
int sos_serial_read(vnode *node, uiovec *iov);

/*
 * Close a serial device
 * @param node, the vnode of the device
 * @param mode, the mode of access held
 * @returns 0 on success else 1
 */
int sos_serial_close(vnode *node, fmode_t mode);

/*
 * Stat the console
 * @param node, the vnode of the device
 * @oaram buf, the stat buffer to write to
 * @returns 0 on success else 1
 */
int sos_serial_stat(vnode *node, sos_stat_t **buf);

#endif /* _SOS_SERIAL_H_ */
