/* Orixbot - built-in.c
 * Felipe Astroza
 * Under GPLv2 (see LICENSE)
 */

/* Comandos y analizadores de mensajes incorportados automaticamente.
 */
#include <string.h>
#include <orix/built-in.h>
#include <orix/modules.h>
#include <orix/parse.h>
#include <orix/log.h>
#include <orix/net.h>
#include <ocore/hash.h>

static void privmsg(orix_msg *msg)
{
	orix_bot *bot = bot_get_current();
	bot_cmd func;
	char *command;

	/* Mensajes publicos */
	if((msg->PM_DEST[0] == '#' || msg->PM_DEST[0] == '&') && msg->PM_CONT[0] == '!' && str_to_list(&command, msg->PM_CONT+1, " \t", 1) ) {

		func = ocore_hash_get_value(&bot->cmds[0], command);
		msg->PM_CONT = command;

		if(func) {
			msg->PM_CONT = command;
			func(msg, strlen(command));
		}
	}
}

static void pong(orix_msg *msg)
{
	send_msg(BOTSOCKET(bot_get_current()), PONG, NULL, "%s", msg->PNG_RESP);
}

void load_builtin(orix_bot *bot)
{
	if(!bot->parsers[PRIVMSG])
		bot->parsers[PRIVMSG] = ocore_dlist_new();

	ocore_dlist_new_node(bot->parsers[PRIVMSG], privmsg);

	if(!bot->parsers[PING])
		bot->parsers[PING] = ocore_dlist_new();

	ocore_dlist_new_node(bot->parsers[PING], pong);

#if ENABLE_DEBUG
	orix_log(DEBUG, "%s(): it's OK", __FUNCTION__);
#endif
}
