#include <stdio.h>
#include <libxml/parser.h>

#include "config.h"

#define MODULE_FILTER
#include "module.h"
#include "image.h"
#include "xmlhelp.h"
#include "log.h"

#define MODNAME "resize"

static int resize_get_dim(struct image *, xmlNodePtr);

char *name = MODNAME;
char *version = VERSION;

int
filter(struct image *img, xmlNodePtr node)
{
	int ret;
	struct image work;
	double xratio, yratio;
	unsigned int x, y, rx, ry;
	unsigned char *r, *rline, *w;
	
	memcpy(&work, img, sizeof(work));
	ret = resize_get_dim(&work, node);
	if (ret)
	{
		log_log(MODNAME, "Invalid/missing resize parameters\n");
		return -1;
	}
	
	w = work.buf;
	xratio = (double) img->x / work.x;
	yratio = (double) img->y / work.y;
	for (y = 0; y < work.y; y++)
	{
		ry = yratio * y;
		rline = img->buf + ry * img->x * 3;
		for (x = 0; x < work.x; x++)
		{
			rx = xratio * x;
			r = rline + rx * 3;
			*w++ = *r++;
			*w++ = *r++;
			*w++ = *r++;
		}
	}
	
	image_move(img, &work);

	return 0;
}

static
int
resize_get_dim(struct image *img, xmlNodePtr node)
{
	int scale;
	
	for (node = node->xml_children; node; node = node->next)
	{
		if (xml_isnode(node, "width"))
			img->x = xml_atoi(node, img->x);
		else if (xml_isnode(node, "height"))
			img->y = xml_atoi(node, img->y);
		else if (xml_isnode(node, "scale"))
		{
			scale = xml_atoi(node, 0);
			if (scale <= 0)
				continue;
			img->x = (img->x * scale) / 100;
			img->y = (img->y * scale) / 100;
		}
	}
	
	if (img->x == 0 || img->y == 0)
		return -1;
		
	image_new(img, img->x, img->y);
	return 0;
}

