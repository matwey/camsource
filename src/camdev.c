#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/videodev.h>
#include <string.h>
#include <errno.h>
#include <libxml/parser.h>

#include "config.h"

#include "camdev.h"
#include "unpalette.h"
#include "xmlhelp.h"

int
camdev_open(struct camdev *camdev, xmlNodePtr node)
{
	struct camdev newcamdev;
	int ret;
	struct video_window vidwin;
	char *path;
	unsigned int x, y, fps;
	
	path = "/dev/video0";
	x = y = fps = 0;
	
	if (node)
	{
		for (node = node->children; node; node = node->next)
		{
			if (xml_isnode(node, "path"))
				path = xml_getcontent_def(node, path);
			else if (xml_isnode(node, "width"))
				x = xml_atoi(node, 0);
			else if (xml_isnode(node, "height"))
				y = xml_atoi(node, 0);
			else if (xml_isnode(node, "fps"))
				fps = xml_atoi(node, 0);
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
		return -1;
	}
	if (!(newcamdev.vidcap.type & VID_TYPE_CAPTURE))
	{
		printf("Video device doesn't support grabbing to memory\n");
		return -1;
	}
	
	memset(&vidwin, 0, sizeof(vidwin));
	if (!x)
		vidwin.width = newcamdev.vidcap.maxwidth;
	else
		vidwin.width = x;
	if (!y)
		vidwin.height = newcamdev.vidcap.maxheight;
	else
		vidwin.height = y;
	
	vidwin.flags |= (fps & 0x3f) << 16;
	
	ret = ioctl(newcamdev.fd, VIDIOCSWIN, &vidwin);
	if (ret != 0)
	{
		printf("ioctl \"set grab window\" failed: %s\n", strerror(errno));
		printf("Check your <camdev> config section for width/height and fps settings\n");
		return -1;
	}
	ret = ioctl(newcamdev.fd, VIDIOCGWIN, &vidwin);
	if (ret != 0)
	{
		printf("ioctl \"get grab window\" failed: %s\n", strerror(errno));
		return -1;
	}
	ioctl(newcamdev.fd, VIDIOCSWIN, &vidwin);
	newcamdev.x = vidwin.width;
	newcamdev.y = vidwin.height;
	
	ret = ioctl(newcamdev.fd, VIDIOCGPICT, &newcamdev.vidpic);
	if (ret != 0)
	{
		printf("ioctl \"get pict props\" failed: %s\n", strerror(errno));
		return -1;
	}
	for (newcamdev.pal = palettes; newcamdev.pal->val >= 0; newcamdev.pal++)
	{
		newcamdev.vidpic.palette = newcamdev.pal->val;
		newcamdev.vidpic.depth = 24; /* ? */
		ioctl(newcamdev.fd, VIDIOCSPICT, &newcamdev.vidpic);
		ioctl(newcamdev.fd, VIDIOCGPICT, &newcamdev.vidpic);
		if (newcamdev.vidpic.palette == newcamdev.pal->val)
			goto palfound;	/* break */
	}
	printf("No common supported palette found\n");
	return -1;
	
palfound:
	memcpy(camdev, &newcamdev, sizeof(*camdev));
	
	return 0;
}

