#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <dlfcn.h>
#include <libxml/parser.h>

#include "config.h"

#include "grab.h"
#include "camdev.h"
#include "configfile.h"
#include "mod_handle.h"
#include "image.h"
#include "unpalette.h"
#include "filter.h"

struct grabimage current_img;

void
grab_thread_init()
{
	pthread_mutex_init(&current_img.mutex, NULL);
	pthread_cond_init(&current_img.request_cond, NULL);
	pthread_cond_init(&current_img.ready_cond, NULL);
}

void *
grab_thread(void *arg)
{
	int ret;
	struct camdev camdev;
	unsigned int bpf;
	unsigned char *imgbuf;
	struct image newimg;
	
	ret = camdev_open(&camdev, "/dev/video0");
	if (ret == -1)
	{
		printf("Error opening video device: %s\n", strerror(errno));
		return NULL;
	}
	
	bpf = (camdev.x * camdev.y) * camdev.pal->bpp;
	imgbuf = malloc(bpf);
	
	printf("Camsource ready, ready to grab images...\n");
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
		current_img.idx++;
		current_img.request = 0;
		pthread_cond_broadcast(&current_img.ready_cond);
		pthread_mutex_unlock(&current_img.mutex);
	}

	return 0;
}

void
grab_glob_filters(struct image *img)
{
	xmlDocPtr doc;
	xmlNodePtr node;

	rwlock_rlock(&configdoc_lock);
	doc = xmlCopyDoc(configdoc, 1);
	rwlock_runlock(&configdoc_lock);
	
	node = xmlDocGetRootElement(doc);
	filter_apply(img, node);
	xmlFreeDoc(doc);
}

void
grab_get_image(struct image *img, unsigned int *idx)
{
	pthread_mutex_lock(&current_img.mutex);
	if (*idx <= current_img.idx)
	{
		current_img.request = 1;
		pthread_cond_signal(&current_img.request_cond);
		pthread_cond_wait(&current_img.ready_cond, &current_img.mutex);
	}
	*idx = current_img.idx;
	image_copy(img, &current_img.img);
	pthread_mutex_unlock(&current_img.mutex);
}

