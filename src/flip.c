#include <stdlib.h>
#include <libxml/parser.h>

#include "config.h"

#define MODULE_FILTER
#include "module.h"
#include "flip.h"
#include "image.h"
#include "xmlhelp.h"

char *name = "flip";

int
filter(struct image *img, xmlNodePtr node)
{
	struct image work;
	unsigned int x, y, vy;
	unsigned char *r, *w;
	int h, v;
	
	h = v = 0;
	
	for (node = node->children; node; node = node->next)
	{
		if (xml_isnode(node, "horiz"))
			h = 1;
		else if (xml_isnode(node, "vert"))
			v = 1;
	}
	
	if (!h && !v)
		return 0;
	
	image_dup(&work, img);

	/* 3x3
	 * 0/00 RGBRGBRGB
	 * 1/09 RGBRGBRGB
	 * 2/18 RGBRGBRGB */
	
	r = img->buf;
	for (y = 0; y < img->y; y++)
	{
		if (v)
			vy = img->y - y - 1;
		else
			vy = y;
			
		if (h)
			w = work.buf + (vy + 1) * work.x * 3 - 3;
		else
			w = work.buf + vy * work.x * 3;
		
		for (x = 0; x < img->x; x++)
		{
			w[0] = *r++;
			w[1] = *r++;
			w[2] = *r++;
			if (h)
				w -= 3;
			else
				w += 3;
		}
	}
	
	image_move(img, &work);
	
	return 0;
}

