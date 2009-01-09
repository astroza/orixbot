/* Orixbot - message.h
 * Felipe Astroza
 * Under GPLv2 (see LICENSE)
 */

#ifndef __MESSAGE_H_
#define __MESSAGE_H_

#define PRIVMSG         	0
#define NOTICE          	1
#define PING            	2
#define PONG            	PING
#define PART            	3
#define JOIN            	4
#define MODE            	5
#define QUIT            	6
#define IERROR          	7
#define KICK            	8
#define NICK 			9
#define WHOREPLY		10
#define NAMREPLY		11
#define CHANOPRIVSNEEDED 	12

#define RPL_WHOREPLY 		352
#define RPL_NAMREPLY 		353
#define ERR_CHANOPRIVSNEEDED 	482

#define MESSAGE_TYPES		13 /* Cantidad de tipos de mensajes enumerados aqui */

#define MESSAGE_TOP		MESSAGE_TYPES

#endif
