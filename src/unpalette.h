#ifndef _UNPALETTE_H_
#define _UNPALETTE_H_

typedef void unpalettizer(unsigned int, unsigned int, unsigned char *, unsigned char *);

struct palette
{
	int val;
	unpalettizer *routine;
	double bpp;	/* bytes per pixel */
};

extern struct palette palettes[];

unpalettizer unpalette_stub;
unpalettizer unpalette_yuv420p;

#endif

