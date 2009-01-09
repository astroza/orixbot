/* Orixbot - signal.c
 * Felipe Astroza
 * Under GPLv2 (see LICENSE)
 */

#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#include <orix/modules.h>
#include <orix/log.h>

static void sighandler (int signal) 
{
	switch(signal) {

	case SIGINT:
		
		orix_log(ORIX, "%d Terminated (SIGINT)", getpid());
		break;

	case SIGSEGV:

		orix_log(DEBUG, "%d SIGSEGV!", getpid());
		exit(EXIT_FAILURE);
	}
}


void signals_init ()
{
	struct sigaction sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = sighandler;

	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGSEGV, &sa, NULL);
}

