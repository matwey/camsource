#include <stdlib.h>

#include "config.h"

#define MODULE_FILTER
#include "module.h"
#include "flip.h"
#include "grab.h"

char *name = "flip";
char *deps[] = { NULL };

int
filter(struct image *dst, const struct image *src)
{
	unsigned int x, y;
	unsigned char *r, *w;
	
	/* 3x3
	 * 0/00 RGBRGBRGB
	 * 1/09 RGBRGBRGB
	 * 2/18 RGBRGBRGB */
	dst->x = src->x;
	dst->y = src->y;
	dst->bufsize = src->bufsize;
	dst->buf = malloc(dst->bufsize);
	
	r = src->buf;
	for (y = 0; y < src->y; y++)
	{
		w = dst->buf + (y + 1) * dst->x * 3;
		for (x = 0; x < src->x; x++)
		{
			w -= 3;
			w[0] = *r++;
			w[1] = *r++;
			w[2] = *r++;
		}
	}
	
	return 0;
}

