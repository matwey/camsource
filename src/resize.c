#include <stdio.h>
#include <libxml/parser.h>

#include "config.h"

#define MODULE_FILTER
#include "module.h"
#include "resize.h"
#include "image.h"

char *name = "resize";
char *deps[] = { NULL };

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
		printf("Invalid resize parameters\n");
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

int
resize_get_dim(struct image *img, xmlNodePtr node)
{
	for (node = node->children; node; node = node->next)
	{
		if (xmlStrEqual(node->name, "width") && node->children && node->children->content)
			img->x = atoi(node->children->content);
		else if (xmlStrEqual(node->name, "height") && node->children && node->children->content)
			img->y = atoi(node->children->content);
	}
	
	if (img->x == 0 || img->y == 0)
		return -1;
		
	image_new(img, img->x, img->y);
	return 0;
}

