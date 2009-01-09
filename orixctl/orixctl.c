/* Orixbot - orixctl.c
 * Felipe Astroza
 * Under GPLv2 (see LICENSE)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#define UNIX_DOMAIN "/tmp/orix.socket"

#define OP_LOADBOT 1
#define OP_UNLOADBOT 2
#define OP_ORIXSTOP 3

extern char *__progname;

struct {
	char opcode;
	char *name;
	char flags;
} cmds[] = {
	{OP_LOADBOT, "load", 1},
	{OP_UNLOADBOT, "unload", 1},
	{OP_ORIXSTOP, "stop", 0}
};

#define NUM 3

void help()
{
	fprintf(stdout, "%s: <*command> <arg> <socket>\n", __progname);
	exit(0);
}

int wait_response(int sock, int sec)
{
	fd_set rfds;
	struct timeval tv;
	char r;

	FD_ZERO(&rfds);
	FD_SET(sock, &rfds);
	tv.tv_sec = sec;
	tv.tv_usec = 0;

	if(select(sock + 1, &rfds, NULL, NULL, &tv) <= 0)
		return 0;

	if(recv(sock, &r, 1, 0) <= 0)
		return 0;

	return r;
}
	
int main(int c, char **v)
{
	struct sockaddr_un to;
	int sock, i;
	char *address;
	char buffer[109];
	int length = 1;

	if(c < 2)
		help();

	for(i = 0; i < NUM; i++) {
		if(strcasecmp(cmds[i].name, v[1]) == 0) {
			if(cmds[i].flags) {
				if(c < 3)
					help();
				else {
					strncpy(buffer + 1, v[2], 108);
					length += strlen(buffer + 1);
				}
			}
			buffer[0] = cmds[i].opcode;
			break;
		}
	}

	if(i == NUM+1)
		help();
	
	if(length > 1) /* Hay un argumento */
		i = 3;
	else
		i = 2;

	if(c < i+1)
		address = UNIX_DOMAIN;
	else
		address = v[i];

	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if(sock == -1) {
		perror("socket");
		exit(-1);
	}

	to.sun_family = AF_UNIX;
	strncpy(to.sun_path, address, 108);
	to.sun_path[107] = 0;

	if(connect(sock, (struct sockaddr *)&to, sizeof(to)) == -1) {
		perror("connect");
		exit(-1);
	}

	if(send(sock, buffer, length, 0) == -1) {
		perror("send");
		exit(-1);
	}
	
	if(wait_response(sock, 3))
		printf("OK\n");
	else
		printf("ERROR\n");

	close(sock);
	return 0;
}
