/* Orixbot - bot.h
 * Felipe Astroza
 * Under GPLv2 (see LICENSE)
 */

#ifndef _BOT_H_
#define _BOT_H_

#include <stdio.h>

#include <mplx2.h>
#include <orix/message.h>
#include <orix/parse.h>
#include <ocore/hash.h>
#include <ocore/list.h>

#include <sqlite3.h>

#define BOTSOCKET(a) ((a)->conn->sockfd)

typedef struct {
	struct mplx_socket *conn;

	char *user;
	char *nick;
	char *db_filename;
	char *server_hostname;
	short server_port;

	/* Caracteristicas */
	ocore_hash cmds[2];
	ocore_dlist *parsers[MESSAGE_TYPES];

	/* Clientes activos del bot */
	ocore_dlist *clients;

	/* database */
	sqlite3 *db;

} orix_bot;

int bot_event(struct mplx_list *);
orix_bot *new_bot(const char *, const char *, const char *, const char *, short);
void bot_terminated(struct mplx_socket *);
void bot_destroy(orix_bot *);
int bot_add(orix_bot *);
void bot_kill(orix_bot *);
orix_bot *bot_find(const char *);
ocore_dlist *bot_get_list();
orix_bot *bot_get_current();
int bot_connect(orix_bot *);

#endif
