#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <dlfcn.h>
#include <libxml/parser.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

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
static struct grabthread *find_thread(const char *);
static void *grab_thread(void *);

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

struct grabthread {
	struct grabthread *next;
	char *name;
	struct grabimage curimg;
	struct camdev camdev;
	xmlNodePtr node;
};
static struct grabthread *grabthreadshead;

int
grab_threads_init()
{
	xmlNodePtr node;
	char *name;
	struct grabthread *newthread;
	int ret = 0;
	
	node = xml_root(configdoc);
	for (node = node->xml_children; node; node = node->next) {
		if (!xml_isnode(node, "camdev"))
			continue;
		name = xml_attrval(node, "name");
		if (!name)
			name = "default";
		if (find_thread(name)) {
			printf("Duplicate <camdev> name '%s', skipping\n", name);
			continue;
		}
		
		newthread = malloc(sizeof(*newthread));
		memset(newthread, 0, sizeof(*newthread));
		newthread->name = name;
		newthread->node = node;

		pthread_mutex_init(&newthread->curimg.mutex, NULL);
		pthread_cond_init(&newthread->curimg.request_cond, NULL);
		pthread_cond_init(&newthread->curimg.ready_cond, NULL);
		
		newthread->next = grabthreadshead;
		grabthreadshead = newthread;
		
		ret++;
	}
	
	return ret;
}

static
struct grabthread *
find_thread(const char *name)
{
	struct grabthread *thread;
	
	for (thread = grabthreadshead; thread; thread = thread->next) {
		if (!strcmp(name, thread->name))
			return thread;
	}
	
	return NULL;
}

int
grab_open_all()
{
	int ret;
	struct grabthread *thread;
	
	for (thread = grabthreadshead; thread; thread = thread->next) {
		ret = camdev_open(&thread->camdev, thread->node);
		if (ret == -1) {
			printf("Failed to open v4l device for <camdev name=\"%s\">\n", thread->name);
			return -1;
		}
	}
	
	return 0;
}

void
grab_start_all(void)
{
	struct grabthread *thread;
	pthread_t grab_tid;
	pthread_attr_t attr;

	for (thread = grabthreadshead; thread; thread = thread->next) {
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(&grab_tid, &attr, grab_thread, thread);
		pthread_attr_destroy(&attr);
	}
}

static
void *
grab_thread(void *arg)
{
	int ret;
	struct grabthread *thread;
	unsigned int bpf;
	unsigned char *imgbuf;
	struct image newimg;
	int usemmap;
	struct video_mbuf mbuf;
	unsigned char *mbufp;
	struct video_mmap mmapctx;
	int mbufframe;
	
	thread = arg;
	
	bpf = (thread->camdev.x * thread->camdev.y) * thread->camdev.pal->bpp;
	imgbuf = malloc(bpf);
	
	printf("Camsource " VERSION " ready to grab images for device '%s'...\n", thread->name);
	usemmap = -1;
	mbufp = NULL;
	mbufframe = 0;
	for (;;)
	{
		pthread_mutex_lock(&thread->curimg.mutex);
		if (!thread->curimg.request)
			pthread_cond_wait(&thread->curimg.request_cond, &thread->curimg.mutex);
		pthread_mutex_unlock(&thread->curimg.mutex);
		
		image_new(&newimg, thread->camdev.x, thread->camdev.y);
		if (usemmap)
		{
			if (usemmap < 0)
			{
				ret = ioctl(thread->camdev.fd, VIDIOCGMBUF, &mbuf);
				if (ret < 0)
				{
nommap:
					printf("Not using mmap interface, falling back to read() (dev '%s')\n", thread->name);
					usemmap = 0;
					goto sysread;
				}
				mbufp = mmap(NULL, mbuf.size, PROT_READ, MAP_PRIVATE, thread->camdev.fd, 0);
				if (mbufp == MAP_FAILED)
					goto nommap;
				usemmap = 1;
				mbufframe = 0;
			}
			memset(&mmapctx, 0, sizeof(mmapctx));
			mmapctx.frame = mbufframe;
			mmapctx.width = thread->camdev.x;
			mmapctx.height = thread->camdev.y;
			mmapctx.format = thread->camdev.pal->val;
			ret = ioctl(thread->camdev.fd, VIDIOCMCAPTURE, &mmapctx);
			if (ret < 0)
			{
unmap:
				munmap(mbufp, mbuf.size);
				goto nommap;
			}
			ret = mbufframe;
			ret = ioctl(thread->camdev.fd, VIDIOCSYNC, &ret);
			if (ret < 0)
				goto unmap;
			thread->camdev.pal->routine(&newimg, mbufp + mbuf.offsets[mbufframe]);
			mbufframe++;
			if (mbufframe >= mbuf.frames)
				mbufframe = 0;
		}
		else
		{
sysread:
			ret = read(thread->camdev.fd, imgbuf, bpf);
			if (ret < 0)
			{
				printf("Error while reading from device '%s': %s\n", thread->name, strerror(errno));
				exit(1);
			}
			if (ret == 0)
			{
				printf("EOF while reading from device '%s'\n", thread->name);
				exit(1);
			}
			if (ret < bpf)
				printf("Short read while reading from device '%s' (%i < %i), continuing anyway\n",
				thread->name, ret, bpf);
			thread->camdev.pal->routine(&newimg, imgbuf);
		}
		
		grab_glob_filters(&newimg);

		pthread_mutex_lock(&thread->curimg.mutex);
		image_move(&thread->curimg.img, &newimg);
		
		thread->curimg.ctx.idx++;
		if (thread->curimg.ctx.idx == 0)
			thread->curimg.ctx.idx++;
		gettimeofday(&thread->curimg.ctx.tv, NULL);
		
		thread->curimg.request = 0;
		pthread_cond_broadcast(&thread->curimg.ready_cond);
		pthread_mutex_unlock(&thread->curimg.mutex);
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
grab_get_image(struct image *img, struct grab_ctx *ctx, const char *name)
{
	struct timeval now;
	int diff;
	struct grabthread *thread;
	
	if (!img)
		return;
	
	thread = find_thread(name);
	if (!thread) {
		printf("Warning: trying to grab from non-existant device '%s'\n", name);
		/* dont crash */
		image_new(img, 1, 1);
		return;
	}
	
	pthread_mutex_lock(&thread->curimg.mutex);
	
	if (ctx)
	{
		if (!ctx->idx || ctx->idx == thread->curimg.ctx.idx)
		{
request:
			thread->curimg.request = 1;
			pthread_cond_signal(&thread->curimg.request_cond);
			pthread_cond_wait(&thread->curimg.ready_cond, &thread->curimg.mutex);
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

		memcpy(ctx, &thread->curimg.ctx, sizeof(*ctx));
	}
	
	image_copy(img, &thread->curimg.img);
	pthread_mutex_unlock(&thread->curimg.mutex);
}

