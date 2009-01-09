/* Orixbot - bits.h
 * Felipe Astroza
 * Under GPLv2 (see LICENSE)
 */

#ifndef _BITS_H_
#define _BITS_H_

#define IRC_CMD         0x01
#define IRC_CTCP        0x02
#define SERVER_CMD      0x04
#define PUBLIC 		0x08
#define PRIVATE		0x10
#define DETACHED	0x20

/* Modes */
#define HALF_OP         0x01
#define SUPER_OP        0x02
#define OP              0x04
#define VOICE           0x08
#define AWAY		0x10

/* Flags */
#define AUTH		0x01

#endif
