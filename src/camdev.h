#ifndef _CAMDEV_H_
#define _CAMDEV_H_

#include <sys/types.h>
#include <linux/videodev.h>
#include <libxml/parser.h>

struct palette;

struct camdev
{
	int fd;
	struct video_capability vidcap;
	struct video_picture vidpic;
	struct palette *pal;
	unsigned int x, y;	/* grabbing width and height being used */
};

/* Opens v4l device and fills struct with context info. Second arg
 * points to the <camdev> config section to use. On error, prints
 * error msg and returns -1. */
int camdev_open(struct camdev *, xmlNodePtr);

void camdev_capdump(char *);

#endif

