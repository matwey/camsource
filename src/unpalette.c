#include <linux/videodev.h>

#include "config.h"

#include "unpalette.h"

struct palette palettes[] =
{
	{ VIDEO_PALETTE_RGB24,			unpalette_stub,		3		},
	{ VIDEO_PALETTE_RGB24 | 0x80,	unpalette_stub,		3		},
	{ VIDEO_PALETTE_RGB32,			unpalette_stub,		4		},
	{ VIDEO_PALETTE_RGB32 | 0x80,	unpalette_stub,		4		},
	{ VIDEO_PALETTE_YUYV,			unpalette_stub,		2		},
	{ VIDEO_PALETTE_YUV422,			unpalette_stub,		2		},
	{ VIDEO_PALETTE_YUV420,			unpalette_stub,		1.5	},
	{ VIDEO_PALETTE_YUV420P,		unpalette_yuv420p,	1.5	},
	{ -1 }
};

static unsigned char
citb(int i)
{
	if (i >= 256)
		return 255;
	if (i < 0)
		return 0;
	return i;
}

void
unpalette_stub(unsigned int x, unsigned int y, unsigned char *src, unsigned char *dst)
{
}

void
unpalette_yuv420p(unsigned int x, unsigned int y, unsigned char *src, unsigned char *dst)
{
	unsigned char *u, *v, *bu, *bv;
	unsigned int dx, dy, uvc, buvc;
	
	u = src + x * y;
	v = u + (x / 2) * (y / 2);
	
	buvc = 0;
	for (dy = 0; dy < y; dy++)
	{
		bu = u;
		bv = v;
		uvc = 0;
		
		for (dx = 0; dx < x; dx++)
		{
			/**dst++ = citb((int) *src + 1.4075 * ((int) *v - 128));
			*dst++ = citb((int) *src - 0.3455 * ((int) *u - 128) - 0.7169 * ((int) *v - 128));
			*dst++ = citb((int) *src++ + 1.7790 * ((int) *u - 128));*/
			*dst++ = citb((int) *src + 1.140 * ((int) *v - 128));
			*dst++ = citb((int) *src - 0.396 * ((int) *u - 128) - 0.581 * ((int) *v - 128));
			*dst++ = citb((int) *src++ + 2.029 * ((int) *u - 128));
			
			uvc++;
			if (uvc >= 2)
			{
				uvc = 0;
				u++;
				v++;
			}
		}

		buvc++;
		if (buvc < 2)
		{
			u = bu;
			v = bv;
		}
		else
			buvc = 0;
	}
}

