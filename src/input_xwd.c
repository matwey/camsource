#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <libxml/parser.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>

#include "config.h"

#define MODULE_INPUT
#include "module.h"
#include "xmlhelp.h"
#include "grab.h"
#include "unpalette.h"
#include "log.h"

#define MODNAME "input_xwd"

/* $Id$ */


char *name = MODNAME;
char *version = VERSION;
char *deps[] = { NULL };




typedef int32_t CARD32;
#define B32

struct xwdheader {
	/* header_size = SIZEOF(XWDheader) + length of null-terminated
	 * window name. */
	CARD32 header_size B32;		

	CARD32 file_version B32;	/* = XWD_FILE_VERSION above */
	CARD32 pixmap_format B32;	/* ZPixmap or XYPixmap */
	CARD32 pixmap_depth B32;	/* Pixmap depth */
	CARD32 pixmap_width B32;	/* Pixmap width */
	CARD32 pixmap_height B32;	/* Pixmap height */
	CARD32 xoffset B32;		/* Bitmap x offset, normally 0 */
	CARD32 byte_order B32;		/* of image data: MSBFirst, LSBFirst */

	/* bitmap_unit applies to bitmaps (depth 1 format XY) only.
	 * It is the number of bits that each scanline is padded to. */
	CARD32 bitmap_unit B32;		

	CARD32 bitmap_bit_order B32;	/* bitmaps only: MSBFirst, LSBFirst */

	/* bitmap_pad applies to pixmaps (non-bitmaps) only.
	 * It is the number of bits that each scanline is padded to. */
	CARD32 bitmap_pad B32;		

	CARD32 bits_per_pixel B32;	/* Bits per pixel */

	/* bytes_per_line is pixmap_width padded to bitmap_unit (bitmaps)
	 * or bitmap_pad (pixmaps).  It is the delta (in bytes) to get
	 * to the same x position on an adjacent row. */
	CARD32 bytes_per_line B32;
	CARD32 visual_class B32;	/* Class of colormap */
	CARD32 red_mask B32;		/* Z red mask */
	CARD32 green_mask B32;		/* Z green mask */
	CARD32 blue_mask B32;		/* Z blue mask */
	CARD32 bits_per_rgb B32;	/* Log2 of distinct color values */
	CARD32 colormap_entries B32;	/* Number of entries in colormap; not used? */
	CARD32 ncolors B32;		/* Number of XWDColor structures */
	CARD32 window_width B32;	/* Window width */
	CARD32 window_height B32;	/* Window height */
	CARD32 window_x B32;		/* Window upper left X coordinate */
	CARD32 window_y B32;		/* Window upper left Y coordinate */
	CARD32 window_bdrwidth B32;	/* Window border width */
};



static int xwd_grab(char *, unsigned char *);



int
opendev(xmlNodePtr node, struct grab_camdev *gcamdev)
{
	for (node = node->xml_children; node; node = node->next)
	{
		if (xml_isnode(node, "command"))
			gcamdev->name = xml_getcontent(node);
	}
	
	if (!gcamdev->name) {
		printf("No <command> in xwd config specified\n");
		return -1;
	}
	
	gcamdev->pal = palettes + 1;
	gcamdev->x = 1280;
	gcamdev->y = 960;
	
	return 0;
}

unsigned char *
input(struct grab_camdev *gcamdev)
{
	static unsigned char xbuf[10*1024*1024];
	
	xwd_grab(gcamdev->name, xbuf);
	
	return xbuf;
}

static
int
xwd_grab(char *cmd, unsigned char *output)
{
	FILE *pp;
	int ret;
	unsigned char buf[1024];
	struct xwdheader xwdh;
	int namelen;
	unsigned char *abuf;
	int readlen;

	pp = popen(cmd, "r");
	if (!pp) {
		log_log(MODNAME, "failed to run cmd '%s' from pipe: %s\n", cmd, strerror(errno));
		return -1;
	}
	
	ret = fread(&xwdh, sizeof(xwdh), 1, pp);
	
	if (ret != 1) {
readerr:
		if (ret < 0)
			log_log(MODNAME, "error while reading from %s pipe: %s\n", cmd, strerror(errno));
		else if (ret == 0)
			log_log(MODNAME, "eof while reading from %s pipe\n", cmd);
		else
			log_log(MODNAME, "short read from %s pipe: %i items\n", cmd, ret);
		
killcloseout:
		/* should forcefully kill the child here */
		pclose(pp);
		return -1;
	}
	
	xwdh.file_version = ntohl(xwdh.file_version);
	if (xwdh.file_version != 7) {
		log_log(MODNAME, "%s output doesn't seem to be in proper xwd format\n", cmd);
		goto killcloseout;
	}
	
	xwdh.header_size = ntohl(xwdh.header_size);
	namelen = xwdh.header_size - sizeof(xwdh); 
	if (namelen <= 0 || namelen >= sizeof(buf)) {
		log_log(MODNAME, "%s output has an xwd headersize which doesnt make sense (%i)\n", cmd, xwdh.header_size);
		goto killcloseout;
	}
	
	xwdh.pixmap_format = ntohl(xwdh.pixmap_format);
	if (xwdh.pixmap_format != 1 && xwdh.pixmap_format != 2) {
		log_log(MODNAME, "unsupported xwd format from %s\n", cmd);
		goto killcloseout;
	}
	
	xwdh.pixmap_depth = ntohl(xwdh.pixmap_depth);
	xwdh.bits_per_pixel = ntohl(xwdh.bits_per_pixel);
	if (xwdh.pixmap_depth != 24
		|| (xwdh.bits_per_pixel != 32 && xwdh.bits_per_pixel != 24))
	{
		log_log(MODNAME, "unsupported xwd format from %s\n", cmd);
		goto killcloseout;
	}
	
	if ((ret = fread(buf, namelen, 1, pp)) != 1)
		goto readerr;
	
	xwdh.ncolors = ntohl(xwdh.ncolors);
	readlen = xwdh.ncolors * 12;
	abuf = malloc(readlen);
	if ((ret = fread(abuf, readlen, 1, pp)) != 1) {
freereaderr:
		free(abuf);
		goto readerr;
	}
	free(abuf);
	
	xwdh.pixmap_width = ntohl(xwdh.pixmap_width);
	xwdh.pixmap_height = ntohl(xwdh.pixmap_height);
	xwdh.bytes_per_line = ntohl(xwdh.bytes_per_line);
	readlen = xwdh.bytes_per_line * (xwdh.pixmap_height - 1) + xwdh.pixmap_width * (xwdh.bits_per_pixel / 8);
	
	abuf = malloc(readlen);
	if ((ret = fread(abuf, readlen, 1, pp)) != 1)
		goto freereaderr;
	
	{
		unsigned char *readp, *writep;
		int x, y;
		int elsize;
		
		elsize = xwdh.bits_per_pixel / 8;
		readp = abuf;
		writep = output;
		for (y = 0; y < xwdh.pixmap_height; y++) {
			readp += xwdh.pixmap_width * elsize;
			for (x = 0; x < xwdh.pixmap_width; x++) {
				readp -= elsize;
				*writep++ = readp[2];
				*writep++ = readp[1];
				*writep++ = readp[0];
			}
			readp += xwdh.bytes_per_line;
		}
	}
	
	free(abuf);
	pclose(pp);
	
	return 0;
}

