#include <libxml/parser.h>

#include "config.h"

#include "http.h"
#define MODULE_THREAD
#include "module.h"
#include "xmlhelp.h"
#include "socket.h"

char *name = "http";
char *deps[] =
{
	"jpeg_comp",
	"socket",
	NULL
};

int
init(struct module_ctx *mod_ctx)
{
	int ret;
	struct http_ctx *http_ctx;
	
	http_ctx = malloc(sizeof(*http_ctx));
	ret = load_conf(http_ctx, mod_ctx->node);
	if (ret)
	{
		free(http_ctx);
		return -1;
	}
	
	http_ctx->listen_fd = socket_listen(http_ctx->port, 0);
	if (http_ctx->listen_fd == -1)
	{
		free(http_ctx);
		return -1;
	}
	
	mod_ctx->custom = http_ctx;
	
	return 0;
}

void *
thread(void *mod_ctx)
{
	return NULL;
}

int
load_conf(struct http_ctx *ctx, xmlNodePtr node)
{
	memset(ctx, 0, sizeof(*ctx));
	
	ctx->listen_fd = -1;
	ctx->port = 9192;
	
	for (node = node->children; node; node = node->next)
	{
		if (xml_isnode(node, "port"))
			ctx->port = xml_atoi(node, ctx->port);
	}
	
	if (ctx->port <= 0 || ctx->port > 0xffff)
	{
		printf("Invalid port %u\n", ctx->port);
		return -1;
	}
	
	return 0;
}

