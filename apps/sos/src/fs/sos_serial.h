/*
 * Wrappers for the serial device
 * 
 * Glenn McGuire & Cameron Lonsdale
 */

#ifndef _SOS_SERIAL_H_
#define _SOS_SERIAL_H_

/*
 * Intialise the console device for sos use
 * Creates a vnode for the device and registers that into the VFS
 * @returns 0 on success else 1
 */
int sos_serial_init(void);

#endif /* _SOS_SERIAL_H_ */
