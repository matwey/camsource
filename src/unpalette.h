#ifndef _UNPALETTE_H_
#define _UNPALETTE_H_

struct image;

typedef void unpalettizer(struct image *, const unsigned char *);

struct palette
{
	int val;
	unpalettizer *routine;
	double bpp;	/* bytes per pixel */
};

extern struct palette palettes[];

unpalettizer unpalette_stub;
unpalettizer unpalette_yuv420p;
unpalettizer unpalette_rgb24;
unpalettizer unpalette_bgr24;
unpalettizer unpalette_rgb32;
unpalettizer unpalette_bgr32;

#endif

