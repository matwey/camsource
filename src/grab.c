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
#include "rwlock.h"
#include "configfile.h"
#include "mod_handle.h"
#include "image.h"
#include "unpalette.h"
#include "filter.h"

struct grabimage current_img;

void
grab_thread_init()
{
	pthread_cond_init(&current_img.cond, NULL);
	rwlock_init(&current_img.lock);
	pthread_mutex_init(&current_img.cond_mutex, NULL);
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
	
	printf("Camsource ready, grabbing images...\n");
	for (;;)
	{
		ret = read(camdev.fd, imgbuf, bpf);
		image_new(&newimg, camdev.x, camdev.y);
		camdev.pal->routine(&newimg, imgbuf);
		
		grab_glob_filters(&newimg);
		
		rwlock_wlock(&current_img.lock);
		image_move(&current_img.img, &newimg);
		current_img.idx++;
		rwlock_wunlock(&current_img.lock);
		pthread_mutex_lock(&current_img.cond_mutex);
		pthread_cond_broadcast(&current_img.cond);
		pthread_mutex_unlock(&current_img.cond_mutex);
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
	if (!doc)
	{
		printf("xmlCopyDoc failed\n");
		return;
	}
	
	node = xmlDocGetRootElement(doc);
	filter_apply(img, node);
	xmlFreeDoc(doc);
}

