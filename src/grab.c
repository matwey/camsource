#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "config.h"

#include "grab.h"
#include "camdev.h"
#include "rwlock.h"

struct image current_image;
pthread_cond_t image_cond;
pthread_mutex_t image_cond_mutex;
struct rwlock image_lock;

void
grab_thread_init()
{
	pthread_cond_init(&image_cond, NULL);
	rwlock_init(&image_lock);
	pthread_mutex_init(&image_cond_mutex, NULL);
}

void *
grab_thread(void *arg)
{
	int ret;
	struct camdev camdev;
	unsigned int bpf, rgbsize;
	unsigned char *imgbuf;
	unsigned char *rgbbuf;
	
	ret = camdev_open(&camdev, "/dev/video0");
	if (ret == -1)
	{
		printf("Error opening video device: %s\n", strerror(errno));
		return 0;
	}
	
	bpf = (camdev.x * camdev.y) * camdev.pal->bpp;
	rgbsize = camdev.x * camdev.y * 3;
	imgbuf = malloc(bpf);
	rgbbuf = malloc(rgbsize);
	rwlock_wlock(&image_lock);
	current_image.x = camdev.x;
	current_image.y = camdev.y;
	current_image.bufsize = rgbsize;
	current_image.buf = malloc(rgbsize);
	rwlock_wunlock(&image_lock);

	for (;;)
	{
		ret = read(camdev.fd, imgbuf, bpf);
		printf("image\n");
		camdev.pal->routine(camdev.x, camdev.y, imgbuf, rgbbuf);
		rwlock_wlock(&image_lock);
		memcpy(current_image.buf, rgbbuf, current_image.bufsize);
		rwlock_wunlock(&image_lock);
		pthread_mutex_lock(&image_cond_mutex);
		pthread_cond_broadcast(&image_cond);
		pthread_mutex_unlock(&image_cond_mutex);
	}

	return 0;
}

void
copy_image(struct image *dst, const struct image *src)
{
	memcpy(dst, src, sizeof(*dst));
	dst->buf = malloc(dst->bufsize);
	memcpy(dst->buf, src->buf, dst->bufsize);
}

