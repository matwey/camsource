#include <linux/videodev.h>

#include "config.h"

#include "unpalette.h"
#include "image.h"

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
unpalette_stub(struct image *dst, const unsigned char *src)
{
}

void
unpalette_yuv420p(struct image *dst, const unsigned char *src)
{
	const unsigned char *u, *v, *bu, *bv;
	unsigned char *dstbuf;
	unsigned int dx, dy, uvc, buvc;
	
	u = src + dst->x * dst->y;
	v = u + (dst->x / 2) * (dst->y / 2);
	dstbuf = dst->buf;
	
	buvc = 0;
	for (dy = 0; dy < dst->y; dy++)
	{
		bu = u;
		bv = v;
		uvc = 0;
		
		for (dx = 0; dx < dst->x; dx++)
		{
			/**dstbuf++ = citb((int) *src + 1.4075 * ((int) *v - 128));
			*dstbuf++ = citb((int) *src - 0.3455 * ((int) *u - 128) - 0.7169 * ((int) *v - 128));
			*dstbuf++ = citb((int) *src++ + 1.7790 * ((int) *u - 128));*/
			*dstbuf++ = citb((int) *src + 1.140 * ((int) *v - 128));
			*dstbuf++ = citb((int) *src - 0.396 * ((int) *u - 128) - 0.581 * ((int) *v - 128));
			*dstbuf++ = citb((int) *src++ + 2.029 * ((int) *u - 128));
			
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

