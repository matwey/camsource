#ifndef _GRAB_H_
#define _GRAB_H_

#include <pthread.h>

#include "rwlock.h"
#include "image.h"

struct grabimage
{
	struct image img;
	unsigned int idx;
	pthread_cond_t cond;
	pthread_mutex_t cond_mutex;
	struct rwlock lock;
};

extern struct grabimage current_img;

void grab_thread_init(void);
void *grab_thread(void *);

void grab_glob_filters(struct image *);

#endif

