/* Orixbot - config.h
 * Felipe Astroza
 * Under GPLv2 (see LICENSE)
 */

#ifndef __CONFIG_H_
#define __CINFIG_H_

#define DEFAULT_ADDR "0.0.0.0"
#define DEFAULT_PORT 3334
#define DEFAULT_UNIXDOMAIN "/tmp/orix.socket"

typedef struct {
	/* Direccion IP donde se listara el puerto */
	char *inet_bindaddr;

	/* Puerto TCP */
	unsigned short inet_port;

	/* Direcciona del unix domain */
	char *unix_bindaddr;

	/* Maximo numero de conexiones */
	int max_connections;
} orix_server;

void cfg_parse_file(const char *);
int cfg_parse_bot_file(const char *file);

#endif
