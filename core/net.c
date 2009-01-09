/* Orixbot - net.c
 * Felipe Astroza
 * Under GPLv2 (see LICENSE)
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <unistd.h>

#include <mplx2/mplx2.h>
#include <orix/private.h>
#include <orix/net.h>
#include <orix/server.h>
#include <orix/log.h>
#include <orix/config.h>
#include <orix/message.h>
#include <orix/defs.h>

static struct mplx_handler mplxh;
/* 'n_status' es la condicion del bucle de net_loop */
static int n_status = 0;

#ifdef __FreeBSD__
#define SOPT	1
#define LEVEL IPPROTO_TCP
#define OPT TCP_NOPUSH
#else
#ifdef linux
#define SOPT    1
#define LEVEL SOL_TCP
#define OPT TCP_CORK
#endif
#endif

static void INIT object_init()
{
	/* A estas alturas es imposible un error */
	net_ctl(NCTL_MPLXINIT, NULL);
}

/* do_send(): No envia el paquete hasta que cork sea 0
*/
long do_send(int sockfd, char *buf, int len, int cork)
{
	int status;
	int optlen;
	if(cork) {
		getsockopt(sockfd, LEVEL, OPT, &status, (socklen_t *)&optlen);
		if(status == 0) {
			status = 1;
			if(setsockopt(sockfd, LEVEL, OPT, &status, sizeof(status)) == -1) {
				orix_log(ERROR, "do_send(): %s", strerror(errno));
				return 0;
			}
		}
	} else if(cork == 0) {
		getsockopt(sockfd, LEVEL, OPT, &status, (socklen_t *)&optlen);
		if(status == 1) {
			status = 0;
			if(setsockopt(sockfd, LEVEL, OPT, &status, sizeof(status)) == -1) {
				orix_log(ERROR, "do_send(): %s", strerror(errno));
				return 0;
			}
		}
	}

	return send(sockfd, buf, len, 0);
}

long send_msg(int sockfd, int type, char *to, const char *fmt, ...)
{
	long ret = 0;
	char msg[BUFSIZE]="";
	va_list args;

	if(fmt) {
        	va_start(args, fmt);
        	vsnprintf(msg, BUFSIZE, fmt, args);
		va_end(args);
	} else {
		orix_log(ERROR, "%s(): fmt is NULL", __FUNCTION__);
		return 0;
	}

	if(type == PRIVMSG || type == NOTICE) {

		switch(type) {
			case PRIVMSG:
				ret += do_send(sockfd, "PRIVMSG ", 8, 1);
				break;
			case NOTICE:
				ret += do_send(sockfd, "NOTICE ", 7, 1);
				break;
		}
		ret += do_send(sockfd, to, strlen(to), 1);
		ret += do_send(sockfd, " :", 2, 1);
		ret += do_send(sockfd, msg, strlen(msg), 1);
		ret += do_send(sockfd, "\n", 1, 0);

	} else if(type == PONG) {

		ret += do_send(sockfd, "PONG :", 6, 1);
		ret += do_send(sockfd, msg, strlen(msg), 1);
		ret += do_send(sockfd, "\n", 1, 0);

	} else if(type == RAW)

		ret += do_send(sockfd, msg, strlen(msg), 0);
	else 
		orix_log(ERROR, "%s: send_msg() %d type unknown", __FUNCTION__, type);

	return ret;
}

static int net_loop()
{
	int ret;
	struct mplx_list *list = &mplxh.list;

	while(n_status) {
		switch(mplx_poll_event(&mplxh) ) {
			case MPLX_ERROR:
				n_status = 0;
				break;
			case MPLX_OK:
				break;
			default:
			case MPLX_ONE_EVENT:
				ret = MPLX_CUR(list)->cb_recv(list);
				if (ret <= MPLX_DO_CLOSE )
					mplx_close_conn(&mplxh, MPLX_CUR(list));
				break;
		}
	}
	return 0;
}

void *net_ctl(int cmd, void *arg)
{
	void *ret=0;
	struct mplx_socket *sock;

	switch(cmd) {
		case NCTL_MPLXINIT:
			/* en x86_64 un "int" no es igual en tamaño a un "void *"
			 * de esta manera me evito problemas de casting.
			 */
#ifdef __x86_64__
#define RCAST(a)	(void *)((unsigned long)(a))
#else
#define RCAST(a) 	(void *)(a)
#endif
			ret = RCAST(mplx_init(&mplxh, MPLX_USE_POLL, 5000));
			break;

		case NCTL_MPLXSTART:
			ret = RCAST(net_loop());
			break;

		case NCTL_MPLXSTOP:
			n_status = 0;
			ret = (void *)1;
			break;

		case NCTL_INITSERVER:
			if(!arg)
				break;

			sock = mplx_listen_inet(&mplxh.list, ((orix_server *)arg)->inet_bindaddr, ((orix_server *)arg)->inet_port);
			if(!sock)
				break;
			mplx_set(sock, MPLX_RECV_CALLBACK, (void *)&server_accept);

			sock = mplx_listen_unix(&mplxh.list, ((orix_server *)arg)->unix_bindaddr);
			if(!sock) 
				break;
			mplx_set(sock, MPLX_RECV_CALLBACK, (void *)&server_accept_unix);

			n_status = 1;
			ret = (void *)1;
			break;

		case NCTL_GETMPLXLIST:
			ret = (void *)&mplxh.list;
			break;

		case NCTL_DISCONNECT:
			mplx_close_conn(&mplxh, (struct mplx_socket *)arg);
			ret = (void *)1;
			break;

		default:
			break;
	}

	return ret;
}

/* nsdispatch buscara este simbolo como atributo para el "caching daemon"
 * si existe no intentara conectarse al caching daemon.
 */

#ifdef __FreeBSD__
void _nss_cache_cycle_prevention_function(void)
{

}
#endif
