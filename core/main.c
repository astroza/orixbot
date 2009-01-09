/* Orixbot - main.c
 * Felipe Astroza
 * Under GPLv2 (see LICENSE)
 */

#include <stdio.h>
#include <stdlib.h>

#include <orix/net.h>
#include <orix/signal.h>
#include <orix/config.h>
#include <orix/log.h>

#include <unistd.h>

extern char *__progname;

static void help(int code)
{
	fprintf(stdout, "Use:\n\t%s -l <logfile> -c <configuration>\n", __progname);
	exit(code);
}

int main(int c, char **v)
{
	int opt;
	char pf = 0;

	orix_log_open(NULL);

	while( (opt = getopt(c, v, "l:c:")) != -1) {
		switch(opt) {
			case 'l':
				orix_log_open(optarg);
				break;
			case 'c':
				cfg_parse_file(optarg);
				pf = 1;
				break;
			case '?':
				help(-1);
		}
	}

	if(!pf)
		help(0);

	/* Definiendo señales */	
	signals_init();

	net_ctl(NCTL_MPLXSTART, NULL);

	orix_log(ORIX, "Terminated...");
	return 0;
}
