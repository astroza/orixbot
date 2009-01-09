/* Orixbot - server.h
 * Felipe Astroza
 * Under GPLv2 (see LICENSE)
 */

#ifndef _SERVER_H_
#define _SERVER_H_

#include <mplx2.h>
#include <orix/user.h>
#include <orix/bot.h>
#include <orix/modules.h>

#define MAX_ARGS 6

#define LOGIN 0
#define CONSOLE 1

typedef struct {
	orix_bot *bot;
	char *username;
	int access;
} server_client;

int server_accept(struct mplx_list *);
int server_accept_unix(struct mplx_list *);
int server_recvmsg(struct mplx_list *);
int prompt(int, server_client *);

void set_max_connections(int);

#endif
