#ifndef _GRAB_H_
#define _GRAB_H_

#include <pthread.h>

#include "image.h"

struct grabimage
{
	pthread_mutex_t mutex;

	struct image img;
	unsigned int idx;
	
	int request;
	pthread_cond_t request_cond;
	pthread_cond_t ready_cond;
};

extern struct grabimage current_img;

void grab_thread_init(void);
void *grab_thread(void *);

void grab_glob_filters(struct image *);

void grab_get_image(struct image *, unsigned int *);

#endif

