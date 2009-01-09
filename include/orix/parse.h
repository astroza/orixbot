/* Orixbot - parse.h
 * Felipe Astroza
 * Under GPLv2 (see LICENSE)
 */

#ifndef __PARSE_H_
#define __PARSE_H_

#define MAX_COMPONENTS 15

/* A continuacion componentes identificables de un mensaje */
/* PRIVMSG */
#define PM_CONT component[3] /* Content */
#define PM_DEST component[2] /* Destination */
#define PM_SRC 	component[0] /* Source */

/* PING..PONG */
#define PNG_RESP component[1]

/* JOIN */
#define JN_CHAN component[2]

/* PART */
#define PRT_CHAN component[2]

/* KICK */
#define KCK_CHAN component[2]
#define KCK_NICK component[3]

/* WHO */
#define WHO_NICK component[7]
#define WHO_USER component[4]
#define WHO_HOST component[5]
#define WHO_CHAN component[3]
#define WHO_MODE component[8]

/* NICK */
#define NCK_NEW_NICK component[2]

/* 486 */
#define M486_CHAN component[3]

typedef struct
{
	int count;
	int cmd_id;
	char *component[MAX_COMPONENTS];
	char *src_nick;
	char *src_user;
	char *src_host;
} orix_msg;

void parse_msg(char *, int, orix_msg *);
void clean_buf(const char *);
int str_to_list(char **, char *, char *, int);
int get_cmd_id(char *);

#endif
