#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <libxml/parser.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "config.h"

#define MODULE_INPUT
#include "module.h"
#include "xmlhelp.h"
#include "grab.h"
#include "unpalette.h"
#include "log.h"
#include "image.h"
#include "jpeg.h"

#define MODNAME "input_jpg"

/* $Id$ */


char *name = MODNAME;
char *version = VERSION;
char *deps[] = {
	"jpeg_comp",
	NULL
};





struct jpg_ctx {
	char *file;
	unsigned long delay;
	struct timeval nextimg;
};



int
opendev(xmlNodePtr node, struct grab_camdev *gcamdev)
{
	struct jpg_ctx ctx;
	int i;

	memset(&ctx, 0, sizeof(ctx));

	for (node = node->xml_children; node; node = node->next)
	{
		if (xml_isnode(node, "file")) {
			gcamdev->name = xml_getcontent(node);
			ctx.file = gcamdev->name;
		}
		else if (xml_isnode(node, "fps")) {
			i = xml_atoi(node, 0);
			if (i <= 0) {
				printf("<fps> of %i makes no sense (plugin %s)\n", i, MODNAME);
				return -1;
			}
			ctx.delay = 1000000 / i;
		}
	}
	
	if (!ctx.file || !ctx.delay) {
		printf("Config for %s module incomplete\n", MODNAME);
		return -1;
	}

	gcamdev->custom = malloc(sizeof(struct jpg_ctx));
	memcpy(gcamdev->custom, &ctx, sizeof(ctx));
	gcamdev->pal = palettes + 1;

	return 0;
}

unsigned char *
input(struct grab_camdev *gcamdev)
{
	int fd;
	struct jpg_ctx *ctx;
	struct stat sb;
	int ret;
	unsigned char *buf;
	struct image img;
	struct jpegbuf jb;
	struct timeval now;
	unsigned long delay;

	ctx = gcamdev->custom;

	gettimeofday(&now, NULL);
	if (!ctx->nextimg.tv_sec)
		memcpy(&ctx->nextimg, &now, sizeof(now));

	if (now.tv_sec < ctx->nextimg.tv_sec
			|| (now.tv_sec == ctx->nextimg.tv_sec && now.tv_usec < ctx->nextimg.tv_usec)) {
		delay = (ctx->nextimg.tv_sec - now.tv_sec) * 1000000 + (ctx->nextimg.tv_usec - now.tv_usec);
		usleep(delay);
	}

	ctx->nextimg.tv_usec += ctx->delay;
	while (ctx->nextimg.tv_usec >= 1000000) {
		ctx->nextimg.tv_usec -= 1000000;
		ctx->nextimg.tv_sec++;
	}

	fd = open(ctx->file, O_RDONLY);
	if (fd < 0) {
		log_log(MODNAME, "Unable to open file '%s': %s\n", ctx->file, strerror(errno));
		return NULL;
	}

	ret = fstat(fd, &sb);
	if (ret) {
		log_log(MODNAME, "Unable to fstat() open file '%s': %s\n", ctx->file, strerror(errno));
		goto outerr;
	}
	if (sb.st_size <= 0) {
		log_log(MODNAME, "Empty file '%s'?\n", ctx->file);
		goto outerr;
	}

	buf = malloc(sb.st_size);
	if (!buf)
		goto outerr2;

	ret = read(fd, buf, sb.st_size);
	if (!ret) {
		log_log(MODNAME, "EOF while reading from file '%s'\n", ctx->file);
		goto outerr2;
	}
	if (ret < 0) {
		log_log(MODNAME, "Error while reading from file '%s': %s\n", ctx->file, strerror(errno));
		goto outerr2;
	}

	close(fd);

	jb.buf = buf;
	jb.bufsize = ret;

	jpeg_decompress(&img, &jb);

	gcamdev->x = img.x;
	gcamdev->y = img.y;

	free(buf);
	return img.buf;

outerr2:
	free(buf);
outerr:
	close(fd);
	return NULL;
}
