#include <stdio.h>
#include <sys/types.h>
#include <linux/videodev.h>

#include "config.h"

#include "unpalette.h"
#include "image.h"

static unsigned char citb(int);

static unpalettizer unpalette_stub;
static unpalettizer unpalette_yuv420p;
static unpalettizer unpalette_rgb24;
static unpalettizer unpalette_bgr24;
static unpalettizer unpalette_rgb32;
static unpalettizer unpalette_bgr32;
static unpalettizer unpalette_grey;

struct palette palettes[] =
{
	{ VIDEO_PALETTE_RGB24,			unpalette_bgr24,		3,		24,	"bgr24"							},
	{ VIDEO_PALETTE_RGB24 | 0x80,	unpalette_rgb24,		3,		24,	"rgb24"							},
	{ VIDEO_PALETTE_RGB32,			unpalette_bgr32,		4,		32,	"bgr32"							},
	{ VIDEO_PALETTE_RGB32 | 0x80,	unpalette_rgb32,		4,		32,	"rgb32"							},
	{ VIDEO_PALETTE_YUYV,			unpalette_stub,		2,		24,	"yuyv (16 bpp)"				},
	{ VIDEO_PALETTE_YUV422,			unpalette_stub,		2,		24,	"yuv422 (16 bpp)"				},
	{ VIDEO_PALETTE_YUV420,			unpalette_stub,		1.5,	24,	"yuv420 (12 bpp)"				},
	{ VIDEO_PALETTE_YUV420P,		unpalette_yuv420p,	1.5,	24,	"yuv420 planar (12 bpp)"	},
	{ VIDEO_PALETTE_GREY,			unpalette_grey,		1,		8,		"grayscale (8 bpp)"			},
	{ -1 }
};

static
unsigned char
citb(int i)
{
	if (i >= 256)
		return 255;
	if (i < 0)
		return 0;
	return i;
}

static
void
unpalette_stub(struct image *dst, const unsigned char *src)
{
	printf("palette recognized but not yet supported!\n");
}

static
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

static
void
unpalette_rgb24(struct image *dst, const unsigned char *src)
{
	memcpy(dst->buf, src, dst->bufsize);
}

static
void
unpalette_bgr24(struct image *dst, const unsigned char *src)
{
	unsigned char *dstbuf, *dstend;
	
	dstbuf = dst->buf;
	dstend = dstbuf + dst->bufsize; 
	while (dstbuf < dstend)
	{
		dstbuf[0] = src[2];
		dstbuf[1] = src[1];
		dstbuf[2] = src[0];
		dstbuf += 3;
		src += 3;
	}
}

static
void
unpalette_rgb32(struct image *dst, const unsigned char *src)
{
	unsigned char *dstbuf, *dstend;
	
	dstbuf = dst->buf;
	dstend = dstbuf + dst->bufsize; 
	while (dstbuf < dstend)
	{
		*dstbuf++ = *src++;
		*dstbuf++ = *src++;
		*dstbuf++ = *src++;
		src++;
	}
}

static
void
unpalette_bgr32(struct image *dst, const unsigned char *src)
{
	unsigned char *dstbuf, *dstend;
	
	dstbuf = dst->buf;
	dstend = dstbuf + dst->bufsize; 
	while (dstbuf < dstend)
	{
		dstbuf[0] = src[2];
		dstbuf[1] = src[1];
		dstbuf[2] = src[0];
		dstbuf += 3;
		src += 4;
	}
}

static
void
unpalette_grey(struct image *dst, const unsigned char *src)
{
	unsigned char *dstbuf, *dstend;
	
	dstbuf = dst->buf;
	dstend = dstbuf + dst->bufsize; 
	while (dstbuf < dstend)
	{
		*dstbuf++ = *src;
		*dstbuf++ = *src;
		*dstbuf++ = *src++;
	}
}

