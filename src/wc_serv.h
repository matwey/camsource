#ifndef _WC_SERV_H_
#define _WC_SERV_H_

#include <sys/socket.h>
#include <netinet/in.h>

struct wc_config
{
	int port;
};

struct peer
{
	int fd;
	struct sockaddr_in sin;
};

int load_config(void);
int open_socket(void);
void *handle_conn(void *);

#endif

