#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <libxml/parser.h>

#include "config.h"

#define MODULE_THREAD
#include "module.h"
#include "wc_serv.h"
#include "grab.h"
#include "image.h"
#include "configfile.h"
#include "jpeg.h"
#include "filter.h"
#include "xmlhelp.h"
#include "socket.h"
#include "log.h"

#define MODNAME "wc_serv"

static int wc_load_config(struct wc_ctx *, xmlNodePtr);
static void *wc_handle_conn(void *);

char *name = MODNAME;
char *deps[] =
{
	"jpeg_comp",
	"socket",
	NULL
};

int
init(struct module_ctx *ctx)
{
	int ret;
	struct wc_ctx new_ctx, *wc_ctx;
	
	ret = wc_load_config(&new_ctx, ctx->node);
	if (ret)
		return ret;
	
	wc_ctx = ctx->custom = malloc(sizeof(*wc_ctx));
	memcpy(wc_ctx, &new_ctx, sizeof(*wc_ctx));
		
	ret = socket_listen(wc_ctx->port, 0);
	if (ret == -1)
	{
		log_log(MODNAME, "Failed to open listen socket: %s\n", strerror(errno));
		return -1;
	}
	wc_ctx->listen_fd = ret;

	return 0;
}

void *
thread(void *arg)
{
	struct wc_ctx *ctx;
	struct peer_ctx *peer;
	int ret;
	
	ctx = ((struct module_ctx *) arg)->custom;
	for (;;)
	{
		peer = malloc(sizeof(*peer));
		peer->ctx = arg;
		ret = socket_accept_thread(ctx->listen_fd, &peer->peer, wc_handle_conn, peer);
		if (ret == -1)
		{
			log_log(MODNAME, "accept() error: %s\n", strerror(errno));
			free(peer);
			sleep(1);
			continue;
		}
		/* TODO: keep a list of accepted connections? */
	}
}

static
int
wc_load_config(struct wc_ctx *ctx, xmlNodePtr node)
{
	ctx->port = 8888;
	ctx->listen_fd = -1;

	if (!node)
		return 0;
	
	for (node = node->children; node; node = node->next)
	{
		if (xml_isnode(node, "port"))
			ctx->port = xml_atoi(node, ctx->port);
	}
	
	if (ctx->port <= 0 || ctx->port > 0xffff)
	{
		log_log(MODNAME, "Invalid port: %i\n", ctx->port);
		return -1;
	}

	return 0;
}

static
void *
wc_handle_conn(void *arg)
{
	struct peer_ctx peer;
	int ret;
	char c;
	char buf[1024];
	struct image curimg;
	struct jpegbuf jpegimg;
	int first;
	struct grab_ctx last_idx;
	int count;
	
	memcpy(&peer, arg, sizeof(peer));
	free(arg);
	
	log_log(MODNAME, "New connection from %s:%i\n", socket_ip(&peer.peer), socket_port(&peer.peer));
	
	first = 1;
	count = 0;
	memset(&last_idx, 0, sizeof(last_idx));
	/* TODO: timeout */
	for (;;)
	{
		do
		{
			ret = read(peer.peer.fd, &c, 1);
			if (ret != 1)
				goto closenout;	/* break; break; */
		}
		while (c == '\r');
		
		if (c != '\n')
			break;
		
		if (first)
		{
			/* The webcam_server java applet has a bug in that
			 * it sends two linefeeds before displaying the first
			 * image, with the result that the display is always
			 * one second slow (when using 1 fps). Work around
			 * that. */
			first = 0;
			continue;
		}
	
		grab_get_image(&curimg, &last_idx);
		
		if (peer.ctx->node)
			filter_apply(&curimg, peer.ctx->node);
	
		jpeg_compress(&jpegimg, &curimg, 0);
	
		snprintf(buf, sizeof(buf) - 1,
			"HTTP/1.0 200 OK\n"
			"Content-type: image/jpeg\n"
			"Content-Length: %i\n"
			"Connection: close\n\n",
			jpegimg.bufsize);
		ret = write(peer.peer.fd, buf, strlen(buf));
		if (ret > 0)
			ret = write(peer.peer.fd, jpegimg.buf, jpegimg.bufsize);
		
		image_destroy(&curimg);
		free(jpegimg.buf);
		
		if (ret <= 0)
			break;
		
		count++;
	}

closenout:
	close(peer.peer.fd);

	log_log(MODNAME, "Connection from %s:%i closed, %i frame(s) served\n",
		socket_ip(&peer.peer), socket_port(&peer.peer),
		count);
	
	return NULL;
}

