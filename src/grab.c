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

struct grabimage current_img;
pthread_cond_t current_img_cond;
pthread_mutex_t current_img_cond_mutex;
struct rwlock current_img_lock;

void
grab_thread_init()
{
	pthread_cond_init(&current_img_cond, NULL);
	rwlock_init(&current_img_lock);
	pthread_mutex_init(&current_img_cond_mutex, NULL);
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
	
	for (;;)
	{
		ret = read(camdev.fd, imgbuf, bpf);
		image_new(&newimg, camdev.x, camdev.y);
		camdev.pal->routine(&newimg, imgbuf);
		
		grab_glob_filters(&newimg);
		
		rwlock_wlock(&current_img_lock);
		image_move(&current_img.img, &newimg);
		current_img.idx++;
		rwlock_wunlock(&current_img_lock);
		pthread_mutex_lock(&current_img_cond_mutex);
		pthread_cond_broadcast(&current_img_cond);
		pthread_mutex_unlock(&current_img_cond_mutex);
	}

	return 0;
}

void
grab_glob_filters(struct image *img)
{
	xmlDocPtr doc;
	xmlNodePtr node;
	xmlAttrPtr filtername;
	struct module *mod;
	int (*filter)(struct image *, xmlNodePtr);

	rwlock_rlock(&configdoc_lock);
	doc = xmlCopyDoc(configdoc, 1);
	rwlock_runlock(&configdoc_lock);
	if (!doc)
	{
		printf("xmlCopyDoc failed\n");
		return;
	}
	
	node = xmlDocGetRootElement(doc);
	for (node = node->children; node; node = node->next)
	{
		if (!xmlStrEqual(node->name, "filter"))
			continue;
		filtername = xmlHasProp(node, "name");
		if (!filtername || !filtername->children || !filtername->children->content)
		{
			printf("<filter> without name\n");
			continue;
		}
		rwlock_rlock(&modules_lock);
		mod = mod_find(filtername->children->content);
		rwlock_runlock(&modules_lock);
		if (mod)
		{
			filter = dlsym(mod->dlhand, "filter");
			if (filter)
				filter(img, node);
			else
				printf("Module %s has no \"filter\" routine\n", filtername->children->content);
		}
	}
	xmlFreeDoc(doc);
}

