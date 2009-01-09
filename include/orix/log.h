/* Orixbot - log.h
 * Felipe Astroza
 * Under GPLv2 (see LICENSE)
 */

#ifndef __LOG_H_
#define __LOG_H_

#include <stdarg.h>

#define LOG_BUFSIZE	512

enum LOG_TYPE {
	BOT = 0,
	ORIX,
	DEBUG,
	ERROR
};

void orix_log(enum LOG_TYPE, const char *, ...);
void orix_log_open(const char *);

#endif
