#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/videodev.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <libxml/parser.h>

#include "config.h"

#include "camdev.h"
#include "unpalette.h"
#include "xmlhelp.h"

static int camdev_size_def(xmlNodePtr);
static int camdev_size_set(int, int, int, char *);

int
camdev_open(struct camdev *camdev, xmlNodePtr node)
{
	struct camdev newcamdev;
	int ret;
	struct video_window vidwin;
	char *path;
	int x, y, fps;
	int channel;
	struct video_channel vidchan;
	
	path = "/dev/video0";
	x = y = fps = 0;
	channel = -1;
	
	if (node)
	{
		for (node = node->xml_children; node; node = node->next)
		{
			if (xml_isnode(node, "path"))
				path = xml_getcontent_def(node, path);
			else if (xml_isnode(node, "width"))
				x = camdev_size_def(node);
			else if (xml_isnode(node, "height"))
				y = camdev_size_def(node);
			else if (xml_isnode(node, "fps"))
				fps = xml_atoi(node, 0);
			else if (xml_isnode(node, "channel"))
				channel = xml_atoi(node, -1);
		}
	}
	
	newcamdev.fd = open(path, O_RDONLY);
	if (newcamdev.fd < 0)
	{
		printf("Unable to open %s (%s)\n", path, strerror(errno));
		return -1;
	}
	
	ret = ioctl(newcamdev.fd, VIDIOCGCAP, &newcamdev.vidcap);
	if (ret != 0)
	{
		printf("ioctl \"get video cap\" failed: %s\n", strerror(errno));
closenerr:
		close(newcamdev.fd);
		return -1;
	}
	if (!(newcamdev.vidcap.type & VID_TYPE_CAPTURE))
	{
		printf("Video device doesn't support grabbing to memory\n");
		goto closenerr;
	}
	
	if (channel >= 0)
	{
		memset(&vidchan, 0, sizeof(vidchan));
		vidchan.channel = channel;
		ret = ioctl(newcamdev.fd, VIDIOCSCHAN, &vidchan);
		if (ret)
			printf("ioctl \"set input channel\" failed, continuing anyway: %s\n", strerror(errno));
	}
	
	memset(&vidwin, 0, sizeof(vidwin));
	
	x = camdev_size_set(x, newcamdev.vidcap.minwidth, newcamdev.vidcap.maxwidth, "width");
	y = camdev_size_set(y, newcamdev.vidcap.minheight, newcamdev.vidcap.maxheight, "height");
	if (x <= 0 || y <= 0)
		goto closenerr;
	vidwin.width = x;
	vidwin.height = y;
		
	vidwin.flags |= (fps & 0x3f) << 16;
	
	ret = ioctl(newcamdev.fd, VIDIOCSWIN, &vidwin);
	if (ret != 0)
	{
		printf("ioctl \"set grab window\" failed: %s\n", strerror(errno));
		printf("Check your <camdev> config section for width/height and fps settings\n");
		goto closenerr;
	}
	ret = ioctl(newcamdev.fd, VIDIOCGWIN, &vidwin);
	if (ret != 0)
	{
		printf("ioctl \"get grab window\" failed: %s\n", strerror(errno));
		goto closenerr;
	}
	ioctl(newcamdev.fd, VIDIOCSWIN, &vidwin);
	newcamdev.x = vidwin.width;
	newcamdev.y = vidwin.height;
	
	ret = ioctl(newcamdev.fd, VIDIOCGPICT, &newcamdev.vidpic);
	if (ret != 0)
	{
		printf("ioctl \"get pict props\" failed: %s\n", strerror(errno));
		goto closenerr;
	}
	for (newcamdev.pal = palettes; newcamdev.pal->val >= 0; newcamdev.pal++)
	{
		newcamdev.vidpic.palette = newcamdev.pal->val;
		newcamdev.vidpic.depth = newcamdev.pal->depth;
		ioctl(newcamdev.fd, VIDIOCSPICT, &newcamdev.vidpic);
		ioctl(newcamdev.fd, VIDIOCGPICT, &newcamdev.vidpic);
		if (newcamdev.vidpic.palette == newcamdev.pal->val)
			goto palfound;	/* break */
	}
	printf("No common supported palette found\n");
	goto closenerr;
	
palfound:
	memcpy(camdev, &newcamdev, sizeof(*camdev));
	
	return 0;
}

void
camdev_capdump(char *dev)
{
	int fd, ret;
	struct video_capability vidcap;
	struct video_picture vidpic;
	struct palette *pal;
	
	if (!dev)
		dev = "/dev/video0";
	
	fd = open(dev, O_RDONLY);
	if (fd < 0)
	{
		printf("Unable to open %s (%s)\n", dev, strerror(errno));
		return;
	}

	ret = ioctl(fd, VIDIOCGCAP, &vidcap);
	if (ret < 0)
	{
		printf("ioctl(VIDIOCGCAP) (get video capabilites) failed: %s\n", strerror(errno));
		goto closenout;
	}
	
	printf("Capability info for %s:\n", dev);
	printf("  Name: %s\n", vidcap.name);
	printf("    Can %scapture to memory\n", (vidcap.type & VID_TYPE_CAPTURE) ? "" : "not ");
	printf("    %s a tuner\n", (vidcap.type & VID_TYPE_TUNER) ? "Has" : "Doesn't have");
	printf("    Can%s receive teletext\n", (vidcap.type & VID_TYPE_TELETEXT) ? "" : "not");
	printf("    Overlay is %schromakeyed\n", (vidcap.type & VID_TYPE_CHROMAKEY) ? "" : "not ");
	printf("    Overlay clipping is %ssupported\n", (vidcap.type & VID_TYPE_CLIPPING) ? "" : "not ");
	printf("    Overlay %s frame buffer mem\n", (vidcap.type & VID_TYPE_FRAMERAM) ? "overwrites" : "doesn't overwrite");
	printf("    Hardware image scaling %ssupported\n", (vidcap.type & VID_TYPE_SCALES) ? "" : "not ");
	printf("    Captures in %s\n", (vidcap.type & VID_TYPE_MONOCHROME) ? "grayscale only" : "color");
	printf("    Can capture %s image\n", (vidcap.type & VID_TYPE_SUBCAPTURE) ? "only part of the" : "the complete");
	printf("  Number of channels: %i\n", vidcap.channels);
	printf("  Number of audio devices: %i\n", vidcap.audios);
	printf("  Grabbing frame size:\n");
	printf("    Min: %ix%i\n", vidcap.minwidth, vidcap.minheight);
	printf("    Max: %ix%i\n", vidcap.maxwidth, vidcap.maxheight);
	
	ret = ioctl(fd, VIDIOCGPICT, &vidpic);
	if (ret != 0)
	{
		printf("ioctl(VIDIOCGPICT) (get picture properties) failed: %s\n", strerror(errno));
		goto closenout;
	}
	
	printf("\n");
	printf("Palette information:\n");
	for (pal = palettes; pal->val >= 0; pal++)
	{
		if (pal->val == vidpic.palette)
		{
			printf("  Currenctly active palette: %s with depth %u\n", pal->name, vidpic.depth);
			goto palfound;
		}
	}
	printf("  Currenctly active palette: not found/supported? (value %u, depth %u)\n", vidpic.palette, vidpic.depth);
	
palfound:
	printf("  Probing for supported palettes:\n");
	for (pal = palettes; pal->val >= 0; pal++)
	{
		vidpic.palette = pal->val;
		vidpic.depth = pal->depth;
		ioctl(fd, VIDIOCSPICT, &vidpic);
		ioctl(fd, VIDIOCGPICT, &vidpic);
		if (vidpic.palette == pal->val)
			printf("    Palette \"%s\" supported: Yes, with depth %u\n", pal->name, vidpic.depth);
		else
			printf("    Palette \"%s\" supported: No\n", pal->name);
	}	

closenout:
	close(fd);
}

static
int
camdev_size_def(xmlNodePtr node)
{
	char *s;
	
	s = xml_getcontent_def(node, "max");
	if (!strcmp(s, "max") || !strcmp(s, "maximum") || !strcmp(s, "default"))
		return 0;
	else if (!strcmp(s, "min") || !strcmp(s, "minimum"))
		return -1;
	else
		return xml_atoi(node, 0);
}

static
int
camdev_size_set(int val, int min, int max, char *s)
{
	if (val == 0)
		return max;
	if (val == -1)
		return min;
	if (val < min)
	{
		printf("Invalid grabbing %s according to driver (%i < %i)\n", s, val, min);
		return 0;
	}
	if (val > max)
	{
		printf("Invalid grabbing %s according to driver (%i > %i)\n", s, val, max);
		return 0;
	}
	return val;
}

