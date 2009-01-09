/* Orixbot - server.c
 * Felipe Astroza
 * Under GPLv2 (see LICENSE)
 */

#include <stdlib.h>
#include <string.h>

#include <mplx2/mplx2.h>
#include <orix/user.h>
#include <orix/modules.h>
#include <orix/server.h>
#include <ocore/hash.h>
#include <ocore/list.h>
#include <orix/log.h>
#include <orix/defs.h>
#include <orix/net.h>
#include <orix/config.h>
#include <orix/version.h>
#include <orix/irc.h>

#include <arpa/inet.h>
#include <unistd.h>

#define WELCOME_MESSAGE "Welcome to " ORIX_VERSION

static int conn_count = 0; 
static int max_connections = 0;

static int server_login(struct mplx_list *);
static int server_client_disconnected(struct mplx_socket *);
static int server_cmd_call(struct mplx_socket *, int, char **);
static int recv_opcode(struct mplx_list *);

/* Desconecta los usuarios de un bot eliminado 
*/
static void kick_out(void *sock)
{
	net_ctl(NCTL_DISCONNECT, sock);
}

static int server_console(struct mplx_list *list)
{
	struct mplx_socket *cur = MPLX_CUR(list);
	server_client *client = MPLX_DATA(list);
	char *args[MAX_ARGS];
	char buf[BUFSIZE]="";
	int ret, argc;

	if((ret = recv(cur->sockfd, buf, sizeof(buf)-1, 0)) <= 0)
		return 0;

	clean_buf(buf);
	if(*buf != 0) {
		argc = str_to_list(args, buf, " ", MAX_ARGS);
		ret = server_cmd_call(cur, argc, args);
	} if(ret)
		prompt(cur->sockfd, client);

	return ret;
}

/* El formato de autentificacion es bot:username:password
*/
static int server_login(struct mplx_list *list) 
{
	struct mplx_socket *cur = MPLX_CUR(list);
	server_client *client;
	orix_bot *bot;
	char buf[256]="", *args[3];
	int access;

	if(recv(cur->sockfd, buf, sizeof(buf), 0) <= 0)
		return 0;

	clean_buf(buf);
	if(*buf == 0)
		return 0;

	if(str_to_list(args, buf, ":", 3) < 3)
		return 0;

	bot = bot_find(args[0]);
	if(!bot)
		return 0;

	access = user_auth(bot->db, args[1], args[2]);
	if(access == USER_ERROR)
		return 0;

	client = malloc(sizeof(server_client));
	if(!client) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	client->bot = bot;
	client->username = strdup(args[1]);
	client->access = access;

	if(!bot->clients) {
		bot->clients = ocore_dlist_new();
		ocore_list_set_free_func(OCORE_LIST(bot->clients), kick_out);
	}

	ocore_dlist_new_node(bot->clients, (void *)cur);

	mplx_set(cur, MPLX_SET_DATA, (void *)client);
	mplx_set(cur, MPLX_RECV_CALLBACK, (void *)server_console);
	mplx_set(cur, MPLX_SET_TIMEOUT, (void *)300); /* 5 min */

	prompt(cur->sockfd, client);
	return 1;
}

int server_accept(struct mplx_list *list)
{
	int clifd;
	struct sockaddr *sa;
	struct mplx_socket *cur = MPLX_CUR(list);
	socklen_t sa_size;
	struct mplx_socket *sock;

	sa_size = cur->sa_size;
	sa = calloc(1, sa_size);

	if((clifd = accept(cur->sockfd, sa, &sa_size)) == -1) {
		orix_log(ERROR, "%s(): Cant accept client.", __FUNCTION__);
                return 0;
	}

	if(conn_count && conn_count == max_connections) {
		send_msg(clifd, RAW, NULL, "SERVER FULL!!\n");
		close(clifd);
		return 1;
	}

#ifdef ENABLE_DEBUG
	orix_log(DEBUG, "new client from %s", inet_ntoa(((struct sockaddr_in *)sa)->sin_addr));
#endif
	send_msg(clifd, RAW, NULL, "%s :", WELCOME_MESSAGE);

	sock = mplx_add_socket(list, clifd, (struct sockaddr *)sa, sa_size);
	mplx_set(sock, MPLX_RECV_CALLBACK, (void *)&server_login);
	mplx_set(cur, MPLX_DELETE_SOCK_CALLBACK, (void *)&server_client_disconnected);
	mplx_set(sock, MPLX_SET_TIMEOUT, (void *)8);

	conn_count++;
        return 1;
}

static int server_client_disconnected(struct mplx_socket *mplx) 
{
	server_client *client = mplx->data;

	if(client) {
		send_msg(mplx->sockfd, RAW, NULL, "\nGood bye %s!\n", client->username);
		free(client->username);
		free(client);
	}

#ifdef ENABLE_DEBUG
	orix_log(DEBUG, "%s client disconnected", inet_ntoa( ( (struct sockaddr_in *)mplx->sa )->sin_addr));
#endif
	conn_count--;
	return 1;
}

int server_accept_unix(struct mplx_list *list)
{
	struct mplx_socket *cur = MPLX_CUR(list);
	struct sockaddr *sa;
	socklen_t sa_size;
	int clifd;
	struct mplx_socket *sock;

	sa_size = cur->sa_size;
	sa = calloc(1, sa_size);

	if((clifd = accept(cur->sockfd, sa, &sa_size)) == -1) {
		orix_log(ERROR, "%s(): Cant accept client.", __FUNCTION__);
		return 0;
	}

	sock = mplx_add_socket(list, clifd, (struct sockaddr *)sa, sa_size);
	mplx_set(sock, MPLX_RECV_CALLBACK, (void *)&recv_opcode);

	return 1;
}

static int recv_opcode(struct mplx_list *list)
{
	struct mplx_socket *cur = MPLX_CUR(list);
	char buffer[1+108]; /* Byte para Opcode y 108 bytes para argumento */
	int rlen;
	char ret;
	orix_bot *bot;
	
	rlen = recv(cur->sockfd, buffer, sizeof(buffer)-1, 0);
	if(rlen <= 0)
		return 0;

	buffer[rlen] = 0;

	switch(buffer[0]) {
		case 1:
			orix_log(DEBUG, "Loading %s", buffer + 1);
			/* Carga bot */
			ret = cfg_parse_bot_file(buffer + 1);
			if(send(cur->sockfd, (char *)&ret, 1, 0) == -1)
				orix_log(ERROR, "%s() Cant send response", __FUNCTION__);
			break;
		case 2:
			bot = bot_find(buffer + 1);
			/* Elimina bot */
			if(bot) {
				bot_kill(bot);
				ret = 1;
			} else 
				ret = 0;

			if(send(cur->sockfd, (char *)&ret, 1, 0) == -1)
				orix_log(ERROR, "%s() Cant send response", __FUNCTION__);
			break;
		case 3:
			/* Termina orix */
			net_ctl(NCTL_MPLXSTOP, NULL);

			if(send(cur->sockfd, "\x1", 1, 0) == -1)
				orix_log(ERROR, "%s(): Cant send response", __FUNCTION__);

			break;
		default:
			orix_log(ERROR, "OPCODE=%d not implemented", buffer[0]);
			break;
	}

	return 1;
}
	
int prompt(int sockfd, server_client *client)
{
	int ret;
	ret = send_msg(sockfd, RAW, NULL, "-%s@%s> ", client->username, client->bot->nick);
	if(ret < 0) 
		return 0;

	return ret;
}

static int server_cmd_call(struct mplx_socket *cur, int argc, char **argv) 
{
	server_cmd cmd;
	orix_bot *bot = ((server_client *)cur->data)->bot;

	cmd = ocore_hash_get_value(&bot->cmds[1], argv[0]);
	if(!cmd) {
		send_msg(cur->sockfd, RAW, NULL, "%s: not found\n", argv[0]);
		return 1;
	}

	return cmd(cur, argc, argv);
}

void set_max_connections(int value)
{
	max_connections = value;
}
