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
#include "mod_handle.h"
#include "log.h"

#define MODNAME "http"

static int http_load_conf(struct http_ctx *, xmlNodePtr);
static void *http_conn(void *);
static int http_path_ismatch(xmlNodePtr, char *);
static void http_err(struct peer *, char *);
static int http_get_fps(xmlNodePtr);

char *name = MODNAME;
char *version = VERSION;
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
	ret = http_load_conf(http_ctx, mod_ctx->node);
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
		ret = socket_accept_thread(http_ctx->listen_fd, &http_peer->peer, http_conn, http_peer);
		if (ret)
		{
			free(http_peer);
			log_log(MODNAME, "accept() error: %s\n", strerror(errno));
			sleep(1);
			continue;
		}
	}
	
	return NULL;
}

static
int
http_load_conf(struct http_ctx *ctx, xmlNodePtr node)
{
	memset(ctx, 0, sizeof(*ctx));
	
	ctx->listen_fd = -1;
	ctx->port = 9192;
	
	for (node = node->xml_children; node; node = node->next)
	{
		if (xml_isnode(node, "port"))
			ctx->port = xml_atoi(node, ctx->port);
	}
	
	if (ctx->port <= 0 || ctx->port > 0xffff)
	{
		log_log(MODNAME, "Invalid port %u\n", ctx->port);
		return -1;
	}
	
	return 0;
}

static
void *
http_conn(void *peer_p)
{
	struct http_peer http_peer;
	char buf[1024];
	int ret;
	char *url;
	char *p;
	char *httpver;
	xmlNodePtr subnode;
	struct image img;
	struct grab_ctx idx;
	struct jpegbuf jpegbuf;
	int fps, count;
	
	memcpy(&http_peer, peer_p, sizeof(http_peer));
	free(peer_p);
	
	log_log(MODNAME, "New connection from %s:%i\n",
		socket_ip(&http_peer.peer), socket_port(&http_peer.peer));
	
	count = 0;
	
	ret = socket_readline(&http_peer.peer, buf, sizeof(buf), 20000);
	if (ret)
	{
readlineerr:
		switch (ret)
		{
		case -1:
			log_log(MODNAME, "Read error on connection from %s:%i (%s)\n",
				socket_ip(&http_peer.peer), socket_port(&http_peer.peer), strerror(errno));
			break;
		case -2:
			log_log(MODNAME, "Timeout on connection from %s:%i\n",
				socket_ip(&http_peer.peer), socket_port(&http_peer.peer));
			break;
		case -3:
			log_log(MODNAME, "EOF on connection from %s:%i before full HTTP request was received\n",
				socket_ip(&http_peer.peer), socket_port(&http_peer.peer));
			break;
		}
		goto closenout;
	}
	
	if (strncmp("GET ", buf, 4))
	{
		log_log(MODNAME, "Invalid HTTP request (not GET) from %s:%i\n",
			socket_ip(&http_peer.peer), socket_port(&http_peer.peer));
		goto closenout;
	}
	
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
	{
		log_log(MODNAME, "Invalid HTTP request or version from %s:%i\n",
			socket_ip(&http_peer.peer), socket_port(&http_peer.peer));
		goto closenout;
	}
	
	log_log(MODNAME, "Request for %s from %s:%i\n",
		url, socket_ip(&http_peer.peer), socket_port(&http_peer.peer));

	for (subnode = http_peer.mod_ctx->node->xml_children; subnode; subnode = subnode->next)
	{
		if (!xml_isnode(subnode, "vpath"))
			continue;
		if (!http_path_ismatch(subnode, url))
			continue;
		goto match;
	}
	http_err(&http_peer.peer, "404 Not found");
	goto closenout;
	
match:
	for (;;)
	{
		ret = socket_readline(&http_peer.peer, buf, sizeof(buf), 20000);
		if (ret)
			goto readlineerr;
		if (!*buf)
			break;
	}
	
	fps = http_get_fps(subnode);
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
		socket_write(&http_peer.peer, buf, strlen(buf), 20000);
	}
	
	memset(&idx, 0, sizeof(idx));
	do
	{
		grab_get_image(&img, &idx);
		filter_apply(&img, http_peer.mod_ctx->node);
		filter_apply(&img, subnode);
		jpeg_compress(&jpegbuf, &img, subnode);
		
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
		ret = socket_write(&http_peer.peer, buf, strlen(buf), 20000);
		if (ret > 0)
			ret = socket_write(&http_peer.peer, jpegbuf.buf, jpegbuf.bufsize, 20000);

		count++;

		if (fps && ret > 0)
		{
			ret = socket_write(&http_peer.peer, "\r\n--Juigt9G7bb7sfdgsdZGIDFsjhn\r\n", 32, 20000);
			usleep(1000000 / fps);
		}
		
		image_destroy(&img);
		free(jpegbuf.buf);
	}
	while (fps > 0 && ret > 0);

closenout:
	log_log(MODNAME, "Closing connection from %s:%i, %i frame(s) served\n",
		socket_ip(&http_peer.peer), socket_port(&http_peer.peer),
		count);

	socket_close(&http_peer.peer);
	return NULL;
}

static
int
http_path_ismatch(xmlNodePtr node, char *path)
{
	for (node = node->xml_children; node; node = node->next)
	{
		if (!xml_isnode(node, "path"))
			continue;
		if (!strcmp(path, xml_getcontent_def(node, "")))
			return 1;
	}
	
	return 0;
}

static
void
http_err(struct peer *peer, char *err)
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
	socket_write(peer, buf, strlen(buf), 20000);
}

static
int
http_get_fps(xmlNodePtr node)
{
	for (node = node->xml_children; node; node = node->next)
	{
		if (xml_isnode(node, "fps"))
			return xml_atoi(node, 0);
	}
	return 0;
}

