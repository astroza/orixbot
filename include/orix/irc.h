/* Orixbot - irc.h
 * Felipe Astroza
 * Under GPLv2 (see LICENSE)
 */

#ifndef __MSG_H
#define __MSG_H

#include <orix/channel.h>

#define CTCP_FORMAT(a)	"\x01" a "\x01"

int irc_auth(int, const char *, const char *);
int irc_join(int, const char *);
int irc_part(int, const char *, const char *);
int irc_quit(int, const char *);
int irc_kick(int, const char *, const char *, char *);
void irc_who(int, const char *);

#endif
