#ifndef _WC_SERV_H_
#define _WC_SERV_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <libxml/parser.h>

struct wc_ctx
{
	int port;
	int listen_fd;
};

struct peer
{
	int fd;
	struct sockaddr_in sin;
	struct module_ctx *ctx;
};

int load_config(struct wc_ctx *, xmlNodePtr);
int open_socket(struct wc_ctx *);
void *handle_conn(void *);

#endif

