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
	unsigned int bpf;
	unsigned char *imgbuf;
	struct image newimg, filterimg;
	xmlDocPtr doc;
	xmlNodePtr node;
	char *prop;
	struct module *mod;
	int (*filter)(struct image *, const struct image *);
	
	ret = camdev_open(&camdev, "/dev/video0");
	if (ret == -1)
	{
		printf("Error opening video device: %s\n", strerror(errno));
		return 0;
	}
	
	bpf = (camdev.x * camdev.y) * camdev.pal->bpp;
	imgbuf = malloc(bpf);
	
	newimg.x = camdev.x;
	newimg.y = camdev.y;
	newimg.bufsize = camdev.x * camdev.y * 3;
	newimg.buf = malloc(newimg.bufsize);
	
	rwlock_wlock(&image_lock);
	memcpy(&current_image, &newimg, sizeof(current_image));
	current_image.buf = malloc(current_image.bufsize);
	memset(current_image.buf, 0, current_image.bufsize);
	rwlock_wunlock(&image_lock);

	for (;;)
	{
		ret = read(camdev.fd, imgbuf, bpf);
		printf("image\n");
		camdev.pal->routine(camdev.x, camdev.y, imgbuf, newimg.buf);
		
		rwlock_rlock(&configdoc_lock);
		doc = xmlCopyDoc(configdoc, 1);
		rwlock_runlock(&configdoc_lock);
		if (doc)
		{
			node = xmlDocGetRootElement(doc);
			for (node = node->children; node; node = node->next)
			{
				if (!xmlStrEqual(node->name, "filter"))
					continue;
				prop = xmlGetProp(node, "name");
				if (!prop)
				{
					printf("<filter> without name\n");
					continue;
				}
				rwlock_rlock(&modules_lock);
				mod = mod_find(prop);
				rwlock_runlock(&modules_lock);
				if (mod)
				{
					filter = dlsym(mod->dlhand, "filter");
					if (filter)
					{
						ret = filter(&filterimg, &newimg);
						if (!ret)
						{
							free(newimg.buf);
							memcpy(&newimg, &filterimg, sizeof(newimg));
						}
					}
					else
						printf("Module %s has no \"filter\" routine\n", prop);
				}
				free(prop);
			}
			xmlFreeDoc(doc);
		}
		else
			printf("xmlCopyDoc failed\n");
		
		rwlock_wlock(&image_lock);
		memcpy(current_image.buf, newimg.buf, current_image.bufsize);
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

