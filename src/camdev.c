#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/videodev.h>
#include <string.h>
#include <errno.h>

#include "config.h"

#include "camdev.h"
#include "unpalette.h"

int
camdev_open(struct camdev *camdev, const char *path)
{
	struct camdev newcamdev;
	int ret;
	struct video_window vidwin;
	
	newcamdev.fd = open(path, O_RDONLY);
	if (newcamdev.fd < 0)
		return -1;
	
	ret = ioctl(newcamdev.fd, VIDIOCGCAP, &newcamdev.vidcap);
	if (ret != 0)
		return -1;
	if (!(newcamdev.vidcap.type & VID_TYPE_CAPTURE))
	{
		errno = ENOSYS;
		return -1;
	}
	
	memset(&vidwin, 0, sizeof(vidwin));
	vidwin.width = newcamdev.vidcap.maxwidth;
	vidwin.height = newcamdev.vidcap.maxheight;
	ret = ioctl(newcamdev.fd, VIDIOCSWIN, &vidwin);
	if (ret != 0)
		return -1;
	ret = ioctl(newcamdev.fd, VIDIOCGWIN, &vidwin);
	if (ret != 0)
		return -1;
	newcamdev.x = vidwin.width;
	newcamdev.y = vidwin.height;
	
	ret = ioctl(newcamdev.fd, VIDIOCGPICT, &newcamdev.vidpic);
	if (ret != 0)
		return -1;
	for (newcamdev.pal = palettes; newcamdev.pal->val >= 0; newcamdev.pal++)
	{
		newcamdev.vidpic.palette = newcamdev.pal->val;
		newcamdev.vidpic.depth = 24; /* ? */
		ioctl(newcamdev.fd, VIDIOCSPICT, &newcamdev.vidpic);
		ioctl(newcamdev.fd, VIDIOCGPICT, &newcamdev.vidpic);
		if (newcamdev.vidpic.palette == newcamdev.pal->val)
			goto palfound;	/* break */
	}
	errno = ENOTSUP;
	return -1;
	
palfound:
	memcpy(camdev, &newcamdev, sizeof(*camdev));
	
	return 0;
}

