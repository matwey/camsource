#ifndef _CAMDEV_H_
#define _CAMDEV_H_

#include <linux/videodev.h>
#include <libxml/parser.h>

struct palette;

struct camdev
{
	int fd;
	struct video_capability vidcap;
	struct video_picture vidpic;
	struct palette *pal;
	unsigned int x, y;
};

/* Opens v4l device and fills struct with context info. Returns -1 on error */
int camdev_open(struct camdev *, xmlNodePtr);

#endif

