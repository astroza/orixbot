/* Felipe Astroza */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <mplx2/mplx2.h>
#include <orix/message.h>
#include <orix/modules.h>
#include <orix/channel.h>
#include <orix/bot.h>
#include <orix/net.h>
#include <orix/log.h>

#define SERVER "www.google.com"
#define QUERY_LEN 64

struct google_request {
	char chan[32];
	orix_bot *bot;
};

static void google_end(struct mplx_socket *cur)
{
	free(cur->data);
}

static int google_recv(struct mplx_list *list)
{
	struct mplx_socket *cur = MPLX_CUR(list);
	struct google_request *rq = cur->data;
	char *resp[5];
	char buffer[512];

	if(recv(cur->sockfd, buffer, sizeof(buffer)-1, 0) <= 0) {
		orix_log(ERROR, "%s: No data received from google", __FUNCTION__);
		return 0;
	}

	if(str_to_list(resp, buffer, " \n", 5) < 5) {
		orix_log(ERROR, "%s: Bad response", __FUNCTION__);
		return 1;
	}

	orix_log(DEBUG, "%s: %s %s %s", __FUNCTION__, resp[0], resp[1], resp[2]);

	if(strcmp(resp[1], "302") == 0)
		send_msg(BOTSOCKET(rq->bot), NOTICE, rq->chan, "google search result: %s", resp[4]);
	else
		send_msg(BOTSOCKET(rq->bot), NOTICE, rq->chan, "No result from google");

	return 0;
}

#define min(a,b) ((a) < (b)? (a) : (b))

void google(orix_msg *msg, int namelen)
{
	struct mplx_list *list;
	struct mplx_socket *sockt;
	char *args[] = {NULL};
	struct google_request *rq;
	char query[QUERY_LEN+1], *t, *s, sw, f;
	orix_bot *bot = bot_get_current();

	if(str_to_list(args, msg->PM_CONT + namelen + 1, " \t", 1) == 0) {
		send_msg(BOTSOCKET(bot), PRIVMSG, msg->PM_DEST, "pon !google \"lo que quieres\"");
		return;
	}

	rq = malloc(sizeof(struct google_request));
	query[0] = 0;

	t = args[0];
	s = t;
	sw = 0;
	f = 1;

	while(1) {
		if(isblank(*t) || *t == 0) {
			if(sw) {
				if(!f)
					strncat(query, "%2B", QUERY_LEN);
				else
					f = 0;

				strncat(query, s, min(QUERY_LEN, t - s));
				sw = 0;
			}
			if(*t == 0)
				break;

			s = ++t;
		} else {
			sw = 1;
			t++;
		}
	}

	strncpy(rq->chan, msg->PM_DEST, 32);
	rq->bot = bot;

	list = (struct mplx_list *)net_ctl(NCTL_GETMPLXLIST, NULL);
	sockt = mplx_connect_inet(list, SERVER, 80);
	if(!sockt) {
		orix_log(ERROR, "%s: Unable to connect to %s", __FUNCTION__, SERVER);
		free(rq);
		return;
	}

	mplx_set(sockt, MPLX_RECV_CALLBACK, (void *)&google_recv);
	mplx_set(sockt, MPLX_SET_TIMEOUT, (void *)10);
	mplx_set(sockt, MPLX_DELETE_SOCK_CALLBACK, (void *)&google_end);
	mplx_set(sockt, MPLX_SET_DATA, (void *)rq);

	send_msg(sockt->sockfd, RAW, NULL, "GET /search?hl=en&q=%s&btnI=I%%27m+Feeling+Lucky HTTP/1.0\nHost: %s\nReferer: http://%s/\n\n",
		query, SERVER, SERVER);
}
