#include <stdlib.h>

#include "config.h"

#define MODULE_FILTER
#include "module.h"
#include "flip.h"
#include "image.h"

char *name = "flip";
char *deps[] = { NULL };

int
filter(struct image *img)
{
	struct image work;
	unsigned int x, y;
	unsigned char *r, *w;
	
	image_dup(&work, img);

	/* 3x3
	 * 0/00 RGBRGBRGB
	 * 1/09 RGBRGBRGB
	 * 2/18 RGBRGBRGB */
	
	r = img->buf;
	for (y = 0; y < img->y; y++)
	{
		w = work.buf + (y + 1) * work.x * 3;
		for (x = 0; x < img->x; x++)
		{
			w -= 3;
			w[0] = *r++;
			w[1] = *r++;
			w[2] = *r++;
		}
	}
	
	image_move(img, &work);
	
	return 0;
}

