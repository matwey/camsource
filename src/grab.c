#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <dlfcn.h>
#include <libxml/parser.h>
#include <sys/time.h>

#include "config.h"

#include "grab.h"
#include "camdev.h"
#include "configfile.h"
#include "mod_handle.h"
#include "image.h"
#include "unpalette.h"
#include "filter.h"
#include "xmlhelp.h"

static void grab_glob_filters(struct image *);

/* The grabimage struct and its only instance current_img.
 * (Private data now, not directly accessible by threads.)
 * This is where the grabbing thread stores the frames.
 * Every time a new frame is grabbed, the index is
 * incremented by one. The function grab_get_image is
 * provided to make it easy for a worker thread to read
 * images out of this.
 *
 * The grabber thread works like this:
 *  1) Acquire mutex.
 *  2) If request != 0, skip to step 5.
 *  3) Wait on request_cond, releasing mutex.
 *  4) Wake up from request_cond, reacquiring the mutex.
 *  5) Release mutex.
 *  6) Read frame from device into private local buffer,
 *     convert it to rgb palette, apply any global filters.
 *  7) Acquire mutex.
 *  8) Put new frame into current_img, increase index and
 *     set request = 0;
 *  9) Broadcast signal on ready_cond.
 * 10) Release mutex.
 * 11) Jump back to step 1.
 */
struct grabimage
{
	pthread_mutex_t mutex;

	struct image img;
	struct grab_ctx ctx;
	
	int request;
	pthread_cond_t request_cond;
	pthread_cond_t ready_cond;
};
static struct grabimage current_img;

void
grab_thread_init()
{
	pthread_mutex_init(&current_img.mutex, NULL);
	pthread_cond_init(&current_img.request_cond, NULL);
	pthread_cond_init(&current_img.ready_cond, NULL);
}

struct camdev *
grab_open()
{
	int ret;
	struct camdev *camdev;
	xmlNodePtr node;
	
	camdev = malloc(sizeof(*camdev));

	node = xml_root(configdoc);
	for (node = node->xml_children; node; node = node->next)
	{
		if (xml_isnode(node, "camdev"))
		{
			ret = camdev_open(camdev, node);
			goto camdevopened;
		}
	}
	ret = camdev_open(camdev, NULL);
camdevopened:
	if (ret == -1)
		return NULL;
	
	return camdev;
}

void *
grab_thread(void *arg)
{
	int ret;
	struct camdev camdev;
	unsigned int bpf;
	unsigned char *imgbuf;
	struct image newimg;
	
	memcpy(&camdev, arg, sizeof(camdev));
	free(arg);
	
	bpf = (camdev.x * camdev.y) * camdev.pal->bpp;
	imgbuf = malloc(bpf);
	
	printf("Camsource " VERSION " ready to grab images...\n");
	for (;;)
	{
		pthread_mutex_lock(&current_img.mutex);
		if (!current_img.request)
			pthread_cond_wait(&current_img.request_cond, &current_img.mutex);
		pthread_mutex_unlock(&current_img.mutex);
		
		ret = read(camdev.fd, imgbuf, bpf);
		image_new(&newimg, camdev.x, camdev.y);
		camdev.pal->routine(&newimg, imgbuf);
		
		grab_glob_filters(&newimg);

		pthread_mutex_lock(&current_img.mutex);
		image_move(&current_img.img, &newimg);
		
		current_img.ctx.idx++;
		if (current_img.ctx.idx == 0)
			current_img.ctx.idx++;
		gettimeofday(&current_img.ctx.tv, NULL);
		
		current_img.request = 0;
		pthread_cond_broadcast(&current_img.ready_cond);
		pthread_mutex_unlock(&current_img.mutex);
	}

	return 0;
}

static
void
grab_glob_filters(struct image *img)
{
	xmlNodePtr node;

	node = xml_root(configdoc);
	filter_apply(img, node);
}

void
grab_get_image(struct image *img, struct grab_ctx *ctx)
{
	struct timeval now;
	int diff;
	
	if (!img)
		return;
	
	pthread_mutex_lock(&current_img.mutex);
	
	if (ctx)
	{
		if (!ctx->idx || ctx->idx == current_img.ctx.idx)
		{
request:
			current_img.request = 1;
			pthread_cond_signal(&current_img.request_cond);
			pthread_cond_wait(&current_img.ready_cond, &current_img.mutex);
		}
		else
		{
			gettimeofday(&now, NULL);
			diff = now.tv_sec - ctx->tv.tv_sec;
			if (diff < 0 || diff >= 2)
				goto request;	/* considered harmful (!) */
			diff *= 1000000;
			diff += now.tv_usec - ctx->tv.tv_usec;
			if (diff < 0 || diff >= 500000)
				goto request;
		}

		memcpy(ctx, &current_img.ctx, sizeof(*ctx));
	}
	
	image_copy(img, &current_img.img);
	pthread_mutex_unlock(&current_img.mutex);
}

