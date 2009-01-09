/* Orixbot - parse.c
 * Felipe Astroza
 * Under GPLv2 (see LICENSE)
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <mplx2/mplx2.h>
#include <orix/message.h>
#include <orix/channel.h>
#include <orix/parse.h>

void parse_msg(char *buf, int len, orix_msg *msg)
{
	char *token = buf, *tmp = buf, *end;
	int count = 0;

	/* Segun rfc 1459 los mensajes deben terminar con un par \r\n, en este caso
	 * al que encuentre primero (\r o \n) lo cambiare por un espacio, de esta forma (1) no tendra
	 * problemas con encontrar el ultimo parametro.
	 */
	end = buf + len - 2;
	if(*end == '\r')
		*end = ' ';
	else
		end = NULL;

	memset(msg, 0, sizeof(orix_msg));

	/* Detectando origen (prefijo) */
	if(*buf == ':') {
		while((*token++)) {
			if(*token == ' ') { /* Corto, lo agrego a la lista de componentes del mensaje. */
				*token = 0;
				msg->component[count++] = tmp + 1;
				tmp = (token = token + 1);
				break;
			}
		}
	}

	/* (1) */
	while(*token) {
		if(*token == ' ' && *(token + 1) != ' ') {
			*token = 0; /* Corta la cadena */

			msg->component[count++] = tmp;

			if(*(token + 1) == ':') {
				msg->component[count++] = token + 2;
				if(end) { /* Necesito que los bytes \r\n sean ajustado a 0 */
					*end = 0;
					*(end + 1) = 0;
				}

				break;
			}

			if(count == MAX_COMPONENTS)
				break;

			tmp = token + 1;
		}
		token++;
	}

	if(*buf != ':')
		msg->cmd_id = get_cmd_id(msg->component[0]); /* Si no tiene origen, el comando va primero */
	else {
		msg->cmd_id = get_cmd_id(msg->component[1]);

		tmp = msg->component[0];
		token = tmp;
		/* Separa component[0] (origen) si tiene el formato *!*@*, consiguiendo nick, user, host */
		while((*token++)) {
			if(*token == '!') {
				*token = 0;
				msg->src_nick = tmp;
				tmp = (token = token + 1);
			} else if(*token == '@') {
				*token = 0;
				msg->src_user = tmp;
				msg->src_host = token + 1;
			}
		}
	}
	msg->count = count;
}

int get_cmd_id(char *str) 
{
	if(!str)
		return 0;

	if ( strcasecmp(str, "PRIVMSG") == 0 )
		return PRIVMSG;

	if ( strcasecmp(str, "NOTICE") == 0 )
		return NOTICE;

	if( strcasecmp(str, "PING") == 0 )
		return PING;

	if( strcasecmp(str, "JOIN") == 0 )
		return JOIN;

	if( strcasecmp(str, "PART") == 0 )
		return PART;

	if( strcasecmp(str, "MODE") == 0 )
		return MODE;

	if( strcasecmp(str, "ERROR") == 0 )
		return IERROR;

	if( strcasecmp(str, "QUIT") == 0 )
		return QUIT;

  	if( strcasecmp(str, "KICK") == 0 )
		return KICK;

	if( strcasecmp(str, "NICK") == 0 )
		return NICK;

	if(isdigit(*str))
		return strtol(str, NULL, 10) + MESSAGE_TOP;

	return 0;
}

/* Corta una cadena */
static char *xstrub(char *string, int s, int e)
{
	*(string+e) = 0;
	return (string + s);
}

/* str_to_list():
 * - Pasa una string a una lista, separando por los caracteres de s
 * a menos que este entre un "".
 * - Esta limitado por un maximo 'max' de elementos
 */
int str_to_list(char **list, char *buf, char *s, int max)
{
	int x, i, j, list_idx;
	int len, slen;
	int reg[] = {0, 0, 0};

	if(!buf)
		return 0;

	len = strlen(buf);
	slen = strlen(s);

	for(x=0, list_idx=0, i=0; i < len && list_idx < max; i++) {
		if(memchr(s, buf[i], slen) && reg[0] == 0) {
			j=0;
			while(!memchr(s, buf[i+j], slen))
				j++;
			x = (i = i+j) + 1;
		}
		if(!memchr(s, buf[i], slen) && buf[i] != '"')
			reg[0] = 1;

		if(buf[i] == '"') {
			if(reg[1] == 0 || reg[2] == 1) {
				reg[1] = 1;
				reg[2] = 0;
			} else
				reg[2] = 1;
		}
 
		if(reg[0] == 1 && (reg[1] == 1? reg[2]: 1)) {
			if(memchr(s, buf[i], slen)) {
				list[list_idx++] = xstrub(buf, x+reg[1], i-reg[2]);
				x = i + 1;
			} else 
			if(buf[i+1] == 0) {
				list[list_idx++] = xstrub(buf, x+reg[1], i+1-reg[2]);
				x = i + 1;
			}
			
			if(x > i) {
				reg[0] = 0;
				reg[1] = 0;
				reg[2] = 0;
			}
		}	
	}

	return list_idx;
}

void clean_buf(const char *buf)
{
	char *tmp;

 	if(buf) {
		for(tmp = (char *)buf; *tmp; tmp++)
			if(*tmp == '\n' || *tmp == '\r')
				*tmp = 0;
	}
}
