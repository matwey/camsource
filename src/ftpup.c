#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <libxml/parser.h>

#include "config.h"

#include "ftpup.h"
#define MODULE_THREAD
#include "module.h"
#include "mod_handle.h"
#include "xmlhelp.h"
#include "log.h"
#include "filter.h"
#include "grab.h"
#include "image.h"
#include "jpeg.h"
#include "socket.h"

static int ftpup_load_conf(struct ftpup_ctx *, xmlNodePtr);
static int ftpup_read_ftp_resp(struct peer *);

#define MODNAME "ftpup"

char *name = MODNAME;
char *deps[] =
{
	"jpeg_comp",
	"socket",
	NULL
};



int
init(struct module_ctx *mctx)
{
	struct ftpup_ctx fctx;
	int ret;
	
	ret = ftpup_load_conf(&fctx, mctx->node);
	if (ret)
		return -1;
	
	mctx->custom = malloc(sizeof(fctx));
	memcpy(mctx->custom, &fctx, sizeof(fctx));
	
	return 0;
}

void *
thread(void *mctx)
{
	struct ftpup_ctx *fctx;
	struct image img;
	struct grab_ctx idx;
	struct jpegbuf jbuf;
	struct peer peer, datapeer;
	struct peer localpeer, portpeer;
	int ret;
	int fd;
	char buf[1024];
	int h1, h2, h3, h4, p1, p2;
	
	fctx = ((struct module_ctx *) mctx)->custom;
	
	memset(&idx, 0, sizeof(idx));
	for (;;)
	{
		grab_get_image(&img, &idx);
		filter_apply(&img, ((struct module_ctx *) mctx)->node);
		jpeg_compress(&jbuf, &img, 0);
		
		ret = socket_connect(&peer, fctx->host, fctx->port);
		if (ret)
		{
			log_log(MODNAME, "Connect to %s:%i failed (code %i, errno: %s)\n",
				fctx->host, fctx->port, ret, strerror(errno));
			goto freensleep;
		}
		
		if ((ret = ftpup_read_ftp_resp(&peer)) != 220)
		{
wrongresp:
			log_log(MODNAME, "Connection error or ftp error reply (code %i)\n", ret);
			goto closenstuff;
		}
		
		socket_printf(&peer, "USER %s\r\n", fctx->user);
		if ((ret = ftpup_read_ftp_resp(&peer)) != 331 && ret != 230)
			goto wrongresp;
		
		if (ret != 230)
		{
			socket_printf(&peer, "PASS %s\r\n", fctx->pass);
			if ((ret = ftpup_read_ftp_resp(&peer)) != 230)
				goto wrongresp;
		}
		
		if (fctx->dir)
		{
			socket_printf(&peer, "CWD %s\r\n", fctx->dir);
			if ((ret = ftpup_read_ftp_resp(&peer)) != 250)
				goto wrongresp;
		}
		
		socket_printf(&peer, "TYPE I\r\n");
		if ((ret = ftpup_read_ftp_resp(&peer)) != 200)
			goto wrongresp;

		fd = -1;
		if (!fctx->passive)
		{
			fd = socket_listen(0, 0);
			if (fd < 0)
			{
				log_log(MODNAME, "Error opening listen socket: %s\n", strerror(errno));
				goto closenstuff;
			}
			socket_fill(peer.fd, &localpeer);
			socket_fill(fd, &portpeer);
			socket_printf(&peer, "PORT %u,%u,%u,%u,%u,%u\r\n",
				(localpeer.sin.sin_addr.s_addr >>  0) & 0xff,
				(localpeer.sin.sin_addr.s_addr >>  8) & 0xff,
				(localpeer.sin.sin_addr.s_addr >> 16) & 0xff,
				(localpeer.sin.sin_addr.s_addr >> 24) & 0xff,
				(portpeer.sin.sin_port >> 0) & 0xff,
				(portpeer.sin.sin_port >> 8) & 0xff);
			if ((ret = ftpup_read_ftp_resp(&peer)) != 200)
			{
				close(fd);
				goto wrongresp;
			}
		}
		else
		{
			socket_printf(&peer, "PASV\r\n");
			ret = socket_readline(&peer, buf, sizeof(buf));
			if (ret || strncasecmp(buf, "227 Entering Passive Mode (", 27))
				goto wrongresp;
			ret = sscanf(buf + 27, "%i,%i,%i,%i,%i,%i",
				&h1, &h2, &h3, &h4, &p1, &p2);
			if (ret != 6)
				goto wrongresp;
			sprintf(buf, "%i.%i.%i.%i", h1, h2, h3, h4);
		}
		
		socket_printf(&peer, "STOR %s\r\n", fctx->file);
		
		if (!fctx->passive)
		{
			ret = socket_accept(fd, &datapeer);
			if (ret)
			{
				log_log(MODNAME, "Accept() error: %s\n", strerror(errno));
				close(fd);
				goto closenstuff;
			}
			close(fd);
		}
		else
		{
			ret = socket_connect(&datapeer, buf, p1 << 8 | p2);
			if (ret)
			{
				log_log(MODNAME, "Could not connect to %s in passive mode\n", buf);
				goto closenstuff;
			}
		}

		if ((ret = ftpup_read_ftp_resp(&peer)) != 150)
		{
			socket_close(&datapeer);
			goto wrongresp;
		}
		
		ret = write(datapeer.fd, jbuf.buf, jbuf.bufsize);
		if (ret != jbuf.bufsize)
		{
			log_log(MODNAME, "Write error while uploading: %s\n", strerror(errno));
			socket_close(&datapeer);
			goto closenstuff;
		}
		
		sleep(1);
		socket_close(&datapeer);
		if ((ret = ftpup_read_ftp_resp(&peer)) != 226)
			goto wrongresp;
		
		socket_printf(&peer, "QUIT\r\n");
		ftpup_read_ftp_resp(&peer);

closenstuff:
		socket_close(&peer);
freensleep:
		image_destroy(&img);
		free(jbuf.buf);
		sleep(fctx->interval);
	}
	
	return NULL;
}

static
int
ftpup_load_conf(struct ftpup_ctx *fctx, xmlNodePtr node)
{
	char *s;
	int mult;
	
	if (!node)
		return -1;
	
	memset(fctx, 0, sizeof(*fctx));
	fctx->port = 21;
	fctx->user = "anonymous";
	fctx->pass = "camsource@";
	
	for (node = node->children; node; node = node->next)
	{
		if (xml_isnode(node, "host"))
			fctx->host = xml_getcontent(node);
		else if (xml_isnode(node, "port"))
			fctx->port = xml_atoi(node, fctx->port);
		else if (xml_isnode(node, "user") || xml_isnode(node, "username"))
			fctx->user = xml_getcontent_def(node, fctx->user);
		else if (xml_isnode(node, "pass") || xml_isnode(node, "password"))
			fctx->pass = xml_getcontent_def(node, fctx->pass);
		else if (xml_isnode(node, "dir"))
			fctx->dir = xml_getcontent(node);
		else if (xml_isnode(node, "file"))
			fctx->file = xml_getcontent(node);
		else if (xml_isnode(node, "passive"))
		{
			s = xml_getcontent_def(node, "no");
			if (!strcmp(s, "yes") || !strcmp(s, "on") || !strcmp(s, "1"))
				fctx->passive = 1;
			else
				fctx->passive = 0;
		}
		else if (xml_isnode(node, "safemode"))
		{
			s = xml_getcontent_def(node, "no");
			if (!strcmp(s, "yes") || !strcmp(s, "on") || !strcmp(s, "1"))
				fctx->safemode = 1;
			else
				fctx->safemode = 0;
		}
		else if (xml_isnode(node, "interval"))
		{
			mult = 1;
			s = xml_attrval(node, "unit");
			if (!s)
				s = xml_attrval(node, "units");
			if (s)
			{
				if (!strcmp(s, "sec") || !strcmp(s, "secs") || !strcmp(s, "seconds"))
					;
				else if (!strcmp(s, "min") || !strcmp(s, "mins") || !strcmp(s, "minutes"))
					mult = 60;
				else if (!strcmp(s, "hour") || !strcmp(s, "hours"))
					mult = 3600;
				else if (!strcmp(s, "day") || !strcmp(s, "days"))
					mult = 86400;
				else
				{
					log_log(MODNAME, "Invalid \"unit\" attribute value \"%s\"\n", s);
					return -1;
				}
			}
			fctx->interval = xml_atoi(node, -1) * mult;
		}
	}
	
	if (!fctx->host)
	{
		log_log(MODNAME, "No host specified\n");
		return -1;
	}
	if (fctx->port <= 0 || fctx->port > 0xffff)
	{
		log_log(MODNAME, "Invalid port (%i) specified\n", fctx->port);
		return -1;
	}
	if (!fctx->dir || !fctx->file)
	{
		log_log(MODNAME, "No dir or path specified\n");
		return -1;
	}
	if (fctx->interval <= 0)
	{
		log_log(MODNAME, "No or invalid interval specified\n");
		return -1;
	}

	return 0;
}

static
int
ftpup_read_ftp_resp(struct peer *peer)
{
	int ret;
	char buf[1024];
	
	for (;;)
	{
		ret = socket_readline(peer, buf, sizeof(buf));
		if (ret || strlen(buf) < 3)
			return -1;
		ret = atoi(buf);
		if (!ret)
			return -1;
		if (buf[3] != '-')
			break;
	}
	
	return ret;
}

