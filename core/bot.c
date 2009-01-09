/* Orixbot - bot.c
 * Felipe Astroza
 * Under GPLv2 (see LICENSE)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <mplx2/mplx2.h>
#include <orix/private.h>
#include <orix/defs.h>
#include <orix/bot.h>
#include <orix/modules.h>
#include <orix/log.h>
#include <orix/util.h>
#include <orix/user.h>
#include <orix/channel.h>
#include <orix/net.h>
#include <orix/irc.h>
#include <orix/built-in.h>
#include <ocore/list.h>
#include <ocore/hash.h>

static ocore_dlist *bots_list;

static void free_bot(void *);

/* Rutina al iniciarse */
static INIT void object_init()
{
	bots_list = ocore_dlist_new();
	ocore_list_set_free_func(OCORE_LIST(bots_list), free_bot);
}

/**** Callback para liberar ****/
static void free_bot(void *p)
{
#ifdef ENABLE_DEBUG
	orix_bot *bot = (orix_bot *)p;
	orix_log(DEBUG, "%s: bot's nick:%s", __FUNCTION__, bot->nick);
#endif
	bot_destroy(p);
}

/*********************************/

static inline char get_color(int s)
{
	if(s < 3)
		return 32;
	if(s == 3)
		return 37;

	return 33;
}

static void msg_print(orix_msg *msg, FILE *stream)
{
	int i;

	fprintf(stream, "\033[35m<\033[37m%d\033[35m> ", msg->count);
	fprintf(stream, "[%d] ", msg->cmd_id);

	for(i = 0; i < msg->count; i++)
		fprintf(stream, "\033[%dm<%s> ", get_color(i), msg->component[i]);

	fprintf(stdout, "\n");
	fflush(stream);
}

/* bot_event: Rutina llamada cuando hay un evento en algun servidor de IRC.
 */
int bot_event(struct mplx_list *list)
{
	char buffer[BUFSIZE]="";
	int b = 0;
	char smallbuf;
	orix_msg msg;
	bot_parser parser;
	struct mplx_socket *cur = MPLX_CUR(list);
	orix_bot *bot;
	ocore_list *parsers_list;

	ocore_list_set_current(OCORE_LIST(bots_list), cur->data);
	bot = ocore_list_current(OCORE_LIST(bots_list));

	/* Sanidad */
	if(!bot)
		return 0;

	do {
	       	if(recv(cur->sockfd, &smallbuf, 1, 0) <= 0)
			return 0;

		if(b < BUFSIZE)
			buffer[b++] = smallbuf;

	} while (smallbuf != '\n');

	parse_msg(buffer, b, &msg);
	msg_print(&msg, stdout);

	if(msg.cmd_id < MESSAGE_TYPES) {
		parsers_list = OCORE_LIST(bot->parsers[msg.cmd_id]);
		parser = ocore_list_goto_first(parsers_list);
		while(parser != NULL) {
			parser(&msg);
			parser = ocore_list_next(parsers_list);
		}
	}
#if ENABLE_DEBUG
	else
		orix_log(DEBUG, "%s(): msg.cmd_id=%d not supported", __FUNCTION__, msg.cmd_id);
#endif

	return CONTINUE;
}

int bot_add(orix_bot *new) 
{
	int count;
	orix_bot *bot;
	struct mplx_socket *sock;

	if(sqlite3_open(new->db_filename, &new->db)) {
		orix_log(ERROR, "%s() bot %s: Unable to open database %s", __FUNCTION__, new->nick, new->db_filename);
		sqlite3_close(new->db);
		return 0;
	}

	count = user_get_count(new->db);
	if(count <= 0) {
		if(user_add(new->db, "tmp", "tmp", 0) == USER_ERROR) {
			sqlite3_close(new->db);
			return 0;
		}
	}

	for(bot = ocore_list_goto_first(OCORE_LIST(bots_list)); bot; bot = ocore_list_next(OCORE_LIST(bots_list))) {
		if(strcasecmp(bot->nick, new->nick) == 0) {
			sqlite3_close(new->db);
			return 0;
		}
	}

	sock = mplx_connect_inet(net_ctl(NCTL_GETMPLXLIST, NULL), new->server_hostname, new->server_port);
	if(!sock) {
		orix_log(ERROR, "%s(): Unable to connect bot %s to %s:%d", __FUNCTION__, new->nick,
			new->server_hostname, new->server_port);
		sqlite3_close(new->db);
		return 0;
	}

	load_builtin(new);

	irc_auth(sock->sockfd, new->nick, new->user);

	mplx_set(sock, MPLX_RECV_CALLBACK, (void *)bot_event);
	mplx_set(sock, MPLX_DELETE_SOCK_CALLBACK, (void *)bot_terminated);
	ocore_dlist_new_node(bots_list, new);
	mplx_set(sock, MPLX_SET_DATA, (void *)ocore_list_get_current_ptr(OCORE_LIST(bots_list)));
	new->conn = sock;

#ifdef ENABLE_DEBUG
        orix_log(DEBUG, "%s(): bot added (nick=%s)", __FUNCTION__, new->nick);
#endif
	return 1;
}

/* new_bot(): Asigna la memoria para el nuevo bot (Sin verificadores)
*/
orix_bot *new_bot(const char *nick, const char *user, const char *database, const char *server, short port)
{
	orix_bot *bot;
	unsigned int length[2], i;

	/* bot, user, server, database, son datos constantes (en tamaño) durante la ejecucion del bot.
	 * nick puede cambiar de tamaño por eso lo aislo en la pagina
	 */
	length[0] = strlen(user);
	length[1] = strlen(server);
	bot = malloc(sizeof(orix_bot) + length[0] + length[1] + strlen(database) + 3); /* 3 bytes para 3 cortes :) */
	if(!bot) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	
	bot->user = (char *)(((caddr_t)bot) + sizeof(orix_bot));
	bot->server_hostname = (char *)(((caddr_t)bot->user) + length[0] + 1);
	bot->db_filename = (char *)(((caddr_t)bot->server_hostname) + length[1] + 1);

	ocore_hash_init(&bot->cmds[0], DEFAULT_HASHSIZE, NULL);
	ocore_hash_init(&bot->cmds[1], DEFAULT_HASHSIZE, NULL);

	for(i = 0; i < MESSAGE_TYPES; i++)
		bot->parsers[i] = NULL;

	bot->clients = NULL;

	bot->nick = malloc(NICK_LEN+1);
	if(!bot->nick) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	/* strcpy() incluye el \0 final */
	strncpy(bot->nick, nick, NICK_LEN+1);
	strcpy(bot->user, user);
	strcpy(bot->server_hostname, server);
	strcpy(bot->db_filename, database);
	bot->server_port = port;

	return bot;
}

void bot_terminated(struct mplx_socket *sock)
{
	ocore_dlist_remove_node(bots_list, sock->data);
}

/* bot_destroy(): Libera toda la memoria utilizada por el bot.
 */
void bot_destroy(orix_bot *bot)
{
	unsigned int i;

	if(bot) {
		sqlite3_close(bot->db);

		/* Descargo los modulos que utiliza el bot */
		mod_unload_all_from_bot(bot);

		if(bot->nick)
			free(bot->nick);

		for(i = 0; i < MESSAGE_TYPES; i++)
			if(bot->parsers[i])
				ocore_list_destroy(OCORE_LIST(bot->parsers[i]));

		if(bot->clients)
			ocore_list_destroy(OCORE_LIST(bot->clients));

		ocore_hash_free_table(&bot->cmds[0]);
		ocore_hash_free_table(&bot->cmds[1]);

		free(bot);
	}
}

/* bot_kill: Asesina a un bot conectado
 */
void bot_kill(orix_bot *bot)
{
	/* Esto desencadena en */
	irc_quit(BOTSOCKET(bot), "The Orix Bot rocks!");
}

/* bot_find: Busca bot por nick.
 */
orix_bot *
bot_find(const char *nick)
{
	orix_bot *bot;
	ocore_list_node *old_current;

	/* Salvo la posicion */
	old_current = ocore_list_get_current_ptr(OCORE_LIST(bots_list));

#ifdef ENABLE_DEBUG
	orix_log(DEBUG, "%s: find %s", __FUNCTION__, nick);
#endif
	/* Inicio la busqueda */
	for(bot = ocore_list_goto_first(OCORE_LIST(bots_list)); 
	bot != NULL; 
	bot = ocore_list_next(OCORE_LIST(bots_list))) {
		 if(!strcasecmp(nick, bot->nick))
			break;
	}

	/* Recupero la posicion */
	ocore_list_set_current(OCORE_LIST(bots_list), old_current);

	return bot;
}

orix_bot *bot_get_current()
{
	/* Puntero de bot actual */
	return ocore_list_current(OCORE_LIST(bots_list));
}
