#ifndef _HTTP_H_
#define _HTTP_H_

#include <libxml/parser.h>

struct http_ctx
{
	int listen_fd;
	unsigned int port;
};

int load_conf(struct http_ctx *, xmlNodePtr);

#endif

