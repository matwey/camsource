#ifndef _UNPALETTE_H_
#define _UNPALETTE_H_

struct image;

/* Converts an image from some palette into rgb24 format.
 * The image struct contains width and height of the image,
 * as well as a large enough output buffer.
 */
typedef void unpalettizer(struct image *, const unsigned char *);

/* Array of all known palettes and their conversion routines */
struct palette
{
	int val;
	unpalettizer *routine;
	double bpp;	/* bytes per pixel, can be something like 1.5, in which case
	             * the number of input bytes must be a multiple of 2. */
};
extern struct palette palettes[];

/* TODO: make this extensible with plugins? */

unpalettizer unpalette_stub;
unpalettizer unpalette_yuv420p;
unpalettizer unpalette_rgb24;
unpalettizer unpalette_bgr24;
unpalettizer unpalette_rgb32;
unpalettizer unpalette_bgr32;

#endif

