/* Orixbot - log.c
 * Felipe Astroza
 * Under GPLv2 (see LICENSE)
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include <orix/bot.h>
#include <orix/log.h>

static FILE *logf = NULL;

void orix_log(enum LOG_TYPE type, const char *fmt, ...)
{
	char buf[LOG_BUFSIZE];
	time_t t;
	char *s_time;
	orix_bot *bot;
	va_list args;

	va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);

	if((t=time(NULL)) != -1) {
		s_time = ctime(&t);
		*(s_time + strlen(s_time) - 1) = 0;
	}

	switch (type) {
		case BOT:
			bot = bot_get_current();
			if(bot)
				fprintf(logf, "%s [%s] %s\n", s_time, bot->nick, buf);
			else
				fprintf(logf, "%s [(unknown)] %s\n", s_time, buf);
			break;
		case ERROR:
			fprintf(logf, "%s -ERROR-: %s\n", s_time, buf);
			break;
		case ORIX:
			fprintf(logf, "%s -ORIX-: %s\n", s_time, buf);
			break;
		case DEBUG:
			fprintf(logf, "%s -DEBUG-: %s\n", s_time, buf);
			break;
	}
	fflush(logf);
}

void orix_log_open(const char *file) 
{

	if(file) {
		logf = fopen(file, "w");
		if(!logf) {
			logf = stdout;
			orix_log(ORIX, "%s: unable to open %s", __FUNCTION__, file);
		}
	} else
		logf = stdout;
}
