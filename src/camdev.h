#ifndef _CAMDEV_H_
#define _CAMDEV_H_

#include <linux/videodev.h>

#include "unpalette.h"

struct camdev
{
	int fd;
	struct video_capability vidcap;
	struct video_picture vidpic;
	struct palette *pal;
	unsigned int x, y;
};

/* Opens v4l device and fills struct with context info. Returns -1 and errno on error */
int camdev_open(struct camdev *, const char *);

#endif

