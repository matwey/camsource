#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "config.h"

#define MODULE_THREAD
#include "module.h"
#include "filewrite.h"
#include "mod_handle.h"
#include "xmlhelp.h"
#include "grab.h"
#include "image.h"
#include "jpeg.h"
#include "filter.h"
#include "log.h"

#define MODNAME "filewrite"

static int fw_load_conf(struct fw_ctx *, xmlNodePtr);

char *name = MODNAME;
char *deps[] =
{
	"jpeg_comp",
	NULL
};

int
init(struct module_ctx *mctx)
{
	int ret;
	struct fw_ctx fctx;
	
	memset(&fctx, 0, sizeof(fctx));
	
	ret = fw_load_conf(&fctx, mctx->node);
	if (ret)
		return -1;
	
	mctx->custom = malloc(sizeof(fctx));
	memcpy(mctx->custom, &fctx, sizeof(fctx));
	
	return 0;
}

void *
thread(void *arg)
{
	struct fw_ctx *fctx;
	int fd;
	char buf[1024];
	struct image curimg;
	struct grab_ctx idx;
	struct jpegbuf jbuf;
	int ret;
	
	fctx = ((struct module_ctx *) arg)->custom;
	snprintf(buf, sizeof(buf) - 1, "%s~", fctx->path);
	
	memset(&idx, 0, sizeof(idx));
	for (;;)
	{
		grab_get_image(&curimg, &idx);
		filter_apply(&curimg, ((struct module_ctx *) arg)->node);
		jpeg_compress(&jbuf, &curimg, 0);
		
		fd = open(buf, O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if (fd < 0)
		{
			log_log(MODNAME, "Open of %s failed: %s\n", buf, strerror(errno));
			goto freesleeploop;
		}
		
		if (fctx->chmod != -1)
			fchmod(fd, fctx->chmod);
		
		ret = write(fd, jbuf.buf, jbuf.bufsize);
		if (ret != jbuf.bufsize)
		{
			log_log(MODNAME, "Write to %s failed: %s\n",
				buf, (ret == -1) ? strerror(errno) : "short write");
			close(fd);
			unlink(buf);
			goto freesleeploop;
		}
		
		close(fd);
		ret = rename(buf, fctx->path);
		if (ret != 0)
		{
			log_log(MODNAME "Rename of %s to %s failed: %s\n",
				buf, fctx->path, strerror(errno));
			unlink(buf);
			goto freesleeploop;
		}
		
freesleeploop:
		free(jbuf.buf);
		image_destroy(&curimg);
		sleep(fctx->interval);
	}
}

static
int
fw_load_conf(struct fw_ctx *fctx, xmlNodePtr node)
{
	int mult, val;
	char *s, *s2;
	
	if (!node)
		return -1;
	
	fctx->chmod = -1;
	fctx->interval = -1;
	
	for (node = node->children; node; node = node->next)
	{
		if (xml_isnode(node, "path"))
			fctx->path = xml_getcontent(node);
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
			val = xml_atoi(node, -1);
			if (val <= 0)
			{
				log_log(MODNAME, "Invalid interval (%s) specified\n", xml_getcontent(node));
				return -1;
			}
			fctx->interval = val * mult;
		}
		else if (xml_isnode(node, "chmod") || xml_isnode(node, "mode"))
			fctx->chmod = xml_atoi(node, fctx->chmod);
	}
	
	if (!fctx->path || fctx->interval <= 0)
	{
		log_log(MODNAME, "Either path or interval not specified\n");
		return -1;
	}
	
	if (!strncmp(fctx->path, "~/", 2))
	{
		s = getenv("HOME");
		if (!s)
		{
			log_log(MODNAME, "Invalid path spec: HOME not set\n");
			return -1;
		}
		s2 = malloc(strlen(s) + strlen(fctx->path));
		sprintf(s2, "%s%s", s, fctx->path + 1);
		fctx->path = s2;
	}
	else
		fctx->path = strdup(fctx->path);
	
	return 0;
}

