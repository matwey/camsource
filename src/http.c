#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <libxml/parser.h>

#include "config.h"

#include "http.h"
#define MODULE_THREAD
#include "module.h"
#include "xmlhelp.h"
#include "socket.h"
#include "grab.h"
#include "filter.h"
#include "jpeg.h"

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
	
	if (!mod_ctx->node)
		return -1;
	
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
	int ret;
	struct http_ctx *http_ctx;
	struct http_peer *http_peer;
	
	http_ctx = ((struct module_ctx *) mod_ctx)->custom;
	
	for (;;)
	{
		http_peer = malloc(sizeof(*http_peer));
		http_peer->mod_ctx = mod_ctx;
		ret = socket_accept_thread(http_ctx->listen_fd, &http_peer->peer, conn, http_peer);
		if (ret)
		{
			free(http_peer);
			printf("Accept() error: %s\n", strerror(errno));
			sleep(1);
			continue;
		}
	}
	
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

void *
conn(void *peer_p)
{
	struct http_peer http_peer;
	char buf[1024];
	int ret;
	char *url;
	char *p;
	char *httpver;
	xmlNodePtr subnode;
	struct image img;
	unsigned int idx;
	struct jpegbuf jpegbuf;
	int fps;
	
	memcpy(&http_peer, peer_p, sizeof(http_peer));
	free(peer_p);
	
	ret = socket_readline(http_peer.peer.fd, buf, sizeof(buf));
	if (ret)
		goto closenout;
	
	if (strncmp("GET ", buf, 4))
		goto closenout;
	
	url = buf + 4;
	
	p = strchr(url, ' ');
	if (!p)
		httpver = NULL;
	else
	{
		*p++ = '\0';
		if (strncmp("HTTP/", p, 5))
			httpver = NULL;
		else
		{
			httpver = p + 5;
			if (!*httpver)
				httpver = NULL;
		}
	}
	
	if (!*url || !httpver)
		goto closenout;
	
	for (subnode = http_peer.mod_ctx->node->children; subnode; subnode = subnode->next)
	{
		if (!xml_isnode(subnode, "vpath"))
			continue;
		if (!path_ismatch(subnode, url))
			continue;
		goto match;
	}
	http_err(http_peer.peer.fd, "404 Not found");
	goto closenout;
	
match:
	for (;;)
	{
		ret = socket_readline(http_peer.peer.fd, buf, sizeof(buf));
		if (ret)
			goto closenout;
		if (!*buf)
			break;
	}
	
	fps = get_fps(subnode);
	if (fps > 0)
	{
		snprintf(buf, sizeof(buf) - 1,
			"HTTP/1.0 200 OK\r\n"
			"Server: " PACKAGE_STRING "\r\n"
			"Pragma: no-cache\r\n"
			"Cache-Control: no-cache\r\n"
			"Content-Type: multipart/x-mixed-replace;boundary=Juigt9G7bb7sfdgsdZGIDFsjhn\r\n"
			"\r\n"
			"--Juigt9G7bb7sfdgsdZGIDFsjhn\r\n");
		write(http_peer.peer.fd, buf, strlen(buf));
	}
	
	idx = 0;
	do
	{
		grab_get_image(&img, &idx);
		filter_apply(&img, http_peer.mod_ctx->node);
		filter_apply(&img, subnode);
		jpeg_compress(&jpegbuf, &img, 0);
		
		if (!fps)
			snprintf(buf, sizeof(buf) - 1,
				"HTTP/1.0 200 OK\r\n"
				"Server: " PACKAGE_STRING "\r\n"
				"Content-Length: %i\r\n"
				"Connection: close\r\n"
				"Pragma: no-cache\r\n"
				"Cache-Control: no-cache\r\n"
				"Content-Type: image/jpeg\r\n"
				"\r\n",
				jpegbuf.bufsize);
		else
			snprintf(buf, sizeof(buf) - 1,
				"Content-Length: %i\r\n"
				"Content-Type: image/jpeg\r\n"
				"\r\n",
				jpegbuf.bufsize);
		ret = write(http_peer.peer.fd, buf, strlen(buf));
		if (ret > 0)
			ret = write(http_peer.peer.fd, jpegbuf.buf, jpegbuf.bufsize);
		if (fps && ret > 0)
		{
			snprintf(buf, sizeof(buf) - 1,
				"\r\n--Juigt9G7bb7sfdgsdZGIDFsjhn\r\n");
			ret = write(http_peer.peer.fd, buf, strlen(buf));
			usleep(1000000 / fps);
		}
		
		image_destroy(&img);
		free(jpegbuf.buf);
	}
	while (fps > 0 && ret > 0);

closenout:
	sleep(1);
	close(http_peer.peer.fd);
	return NULL;
}

int
path_ismatch(xmlNodePtr node, char *path)
{
	for (node = node->children; node; node = node->next)
	{
		if (!xml_isnode(node, "path"))
			continue;
		if (!strcmp(path, xml_getcontent_def(node, "")))
			return 1;
	}
	
	return 0;
}

void
http_err(int fd, char *err)
{
	char buf[1024];
	
	snprintf(buf, sizeof(buf) - 1,
		"HTTP/1.0 %s\r\n"
		"Server: " PACKAGE_STRING "\r\n"
		"Content-Length: %i\r\n"
		"Connection: close\r\n"
		"Pragma: no-cache\r\n"
		"Cache-Control: no-cache\r\n"
		"Content-Type: text/html\r\n"
		"\r\n"
		"<html><body>%s</body></html>\r\n",
		err, strlen(err) + 28, err);
	write(fd, buf, strlen(buf));
}

int
get_fps(xmlNodePtr node)
{
	for (node = node->children; node; node = node->next)
	{
		if (xml_isnode(node, "fps"))
			return xml_atoi(node, 0);
	}
	return 0;
}

