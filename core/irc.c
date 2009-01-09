/* Orixbot - irc.c
 * Felipe Astroza
 * Under GPLv2 (see LICENSE)
 */

#include <stdio.h>

#include <orix/net.h>
#include <orix/channel.h>
#include <orix/log.h>


/* irc_auth(): Manda los datos para autentificarse con el servidor
 */
int irc_auth(int sockfd, const char *nick, const char *user)
{
	return send_msg(sockfd, RAW, NULL, "user %s * 0 :%s\nnick %s\n", user, nick, nick);
}

/* irc_join(): Envia un JOIN <channel> 
 */
int irc_join(int sockfd, const char *channel)
{
	return send_msg(sockfd, RAW, NULL, "JOIN %s\n", channel);
}

/* irc_part(): Abandona canal */
int irc_part(int sockfd, const char *channel, const char *bye)
{
	if(bye)
		return(send_msg(sockfd, RAW, NULL, "PART %s :%s\n", channel, bye));
	else
		return(send_msg(sockfd, RAW, NULL, "PART %s\n", channel));
}

int irc_quit(int sockfd, const char *bye)
{
	return send_msg(sockfd, RAW, NULL, "QUIT :%s\n", bye? bye : "Orixbot is cool!!");
}

int irc_kick(int sockfd, const char *chan, const char *nick, char *msg)
{
	int ret;

	if(msg)
		ret = send_msg(sockfd, RAW, NULL, "KICK %s %s :%s\n", chan, nick, msg);
	else
		ret = send_msg(sockfd, RAW, NULL, "KICK %s %s\n", chan, nick);

	return ret;
}

void irc_who(int sockfd, const char *to)
{
        send_msg(sockfd, RAW, NULL, "WHO %s\n", to);
}
