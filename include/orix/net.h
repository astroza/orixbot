/* Orixbot - net.h
 * Felipe Astroza
 * Under GPLv2 (see LICENSE)
 */

#ifndef _NET_H_
#define _NET_H_

#include <orix/bot.h>

#define RAW 0x1a

#define NCTL_MPLXSTART	0x01
#define NCTL_MPLXSTOP	0x02
#define NCTL_INITSERVER	0x03
#define NCTL_MPLXINIT	0x04
#define NCTL_GETMPLXLIST 0x05
#define	NCTL_DISCONNECT 0x06

int send_msg(int, int, char *, const char *, ...);
int send_buf(int, char *, ...);

void *net_ctl(int, void *);

#endif
